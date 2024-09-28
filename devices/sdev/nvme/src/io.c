/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

/* NVMe I/O Wrappers for the Storage Device Abstraction Layer */

#include <nvme/nvme.h>
#include <nvme/request.h>
#include <liblux/sdev.h>
#include <liblux/liblux.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static IORequest *requestQueue = NULL;

/* nvmeEnqueueRequest(): creates an I/O request and adds it to the queue
 * params: none
 * returns: pointer to the I/O request
 */

static IORequest *nvmeEnqueueRequest() {
    IORequest *req = calloc(1, sizeof(IORequest));
    if(!req) return NULL;

    if(!requestQueue) {
        requestQueue = req;
        return req;
    }

    IORequest *list = requestQueue;
    while(list->next) list = list->next;

    list->next = req;
    return req;
}

/* nvmeDequeueLast(): dequeues the last request after an I/O error
 * params: none
 * returns: nothing
 */

static void nvmeDequeueLast() {
    if(!requestQueue) return;

    IORequest *list = requestQueue;
    IORequest *prev = NULL;
    while(list->next) {
        prev = list;
        list = list->next;
    }

    // list is now the last entry, and prev is the previous entry
    // invalidate the pointer and free the last entry
    if(prev) prev->next = NULL;
    else requestQueue = NULL;
    free(list);
}

/* nvmeRead(): handler for read requests from an NVMe drive
 * params: cmd - read command message
 * returns: nothing
 */

void nvmeRead(SDevRWCommand *cmd) {
    NVMEController *drive = nvmeGetDrive(cmd->device >> 16);
    if(!drive) {
        cmd->header.response = 1;
        cmd->header.status = -ENODEV;
        luxSendDependency(cmd);
        return;
    }

    int ns = cmd->device & 0xFFFF;

    // TODO: don't actually enforce everything to be in multiples of sector sizes
    if((cmd->start % drive->nsSectorSizes[ns]) || (cmd->count % drive->nsSectorSizes[ns])) {
        cmd->header.response = 1;
        cmd->header.status = -EIO;
        luxSendDependency(cmd);
        return;
    }

    SDevRWCommand *dst = malloc(sizeof(SDevRWCommand) + cmd->count);
    if(!dst) {
        cmd->header.response = 1;
        cmd->header.status = -ENOMEM;
        luxSendDependency(cmd);
        return;
    }

    IORequest *req = nvmeEnqueueRequest();
    if(!req) {
        free(dst);
        cmd->header.response = 1;
        cmd->header.status = -ENOMEM;
        luxSendDependency(cmd);
        return;
    }

    req->rwcmd = dst;
    memcpy(req->rwcmd, cmd, sizeof(SDevRWCommand));

    // we're being optimistic here
    req->rwcmd->header.response = 1;
    req->rwcmd->header.status = 0;
    req->rwcmd->header.length = sizeof(SDevRWCommand) + cmd->count;

    req->drive = cmd->device >> 16;
    req->ns = cmd->device & 0xFFFF;
    req->id = cmd->syscall;

    // submit the command
    uint64_t lba = cmd->start / drive->nsSectorSizes[ns];
    uint16_t count = cmd->count / drive->nsSectorSizes[ns];

    req->queue = nvmeReadSector(drive, ns, req->id, lba, count, req->rwcmd->buffer);
    if(!req->queue) {
        // I/O error
        luxLogf(KPRINT_LEVEL_WARNING, "I/O error on drive %d ns %d\n", req->drive, req->ns);
        free(dst);
        nvmeDequeueLast();

        cmd->header.response = 1;
        cmd->header.status = -EIO;
        luxSendDependency(cmd);
    }
}