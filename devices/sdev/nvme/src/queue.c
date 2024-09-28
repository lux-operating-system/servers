/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

/* NVMe Command Submission */

#include <liblux/liblux.h>
#include <nvme/nvme.h>
#include <nvme/registers.h>
#include <nvme/nvmcmd.h>
#include <unistd.h>     // sched_yield()
#include <string.h>

/* nvmeSubmitDoorbell(): notifies the controller that a command is available
 * params: drive - drive to notify
 * params: q - queue number, zero for admin queue
 * params: tail - tail value to write into the doorbell
 * returns: nothing
 */

void nvmeSubmitDoorbell(NVMEController *drive, int q, int tail) {
    off_t offset = NVME_DOORBELLS + ((q << 1) * drive->doorbellStride);
    nvmeWrite32(drive, offset, tail & 0xFFFF);
}

/* nvmeCompleteDoorbell(): acknowledges the completion of a command
 * params: drive - drive to acknowledge
 * params: q - queue number
 * params: head - head value to write into the doorbell
 * returns: nothing
 */

void nvmeCompleteDoorbell(NVMEController *drive, int q, int head) {
    off_t offset = NVME_DOORBELLS + (((q << 1) + 1) * drive->doorbellStride);
    nvmeWrite32(drive, offset, head & 0xFFFF);
}

/* nvmePoll(): polls the completion status of a command in a queue
 * params: drive - drive to poll
 * params: q - queue number, zero for admin queue
 * params: id - command identifier
 * params: timeout - timeout in units of scheduler yields, zero to wait forever
 * returns: completion queue entry, NULL on timeout
 */

NVMECompletionQueue *nvmePoll(NVMEController *drive, int q, uint16_t id, int timeout) {
    NVMECompletionQueue *cq;
    int entry, head;
    if(!q) {
        cq = drive->acq;
        entry = drive->adminTail-1;
        if(entry < 0) entry = drive->adminQueueSize-1;

        head = entry + 1;
        if(head >= drive->adminQueueSize) head = 0;
    } else {
        cq = drive->cq[q-1];
        entry = drive->ioTails[q-1] - 1;
        if(entry < 0) entry = drive->ioQSize - 1;

        head = entry + 1;
        if(head >= drive->ioQSize) head = 0;
    }

    int time = 0;
    while(cq[entry].commandID != id) {
        time++;
        if(timeout && (time > timeout))
            return NULL;
        sched_yield();
    }

    // acknowledge the completion doorbell
    nvmeCompleteDoorbell(drive, q, head);
    return &cq[entry];
}

/* nvmeSubmit(): submits a command to an NVMe submission queue
 * params: drive - drive to send the command to
 * params: q - queue number, zero for admin queue
 * params: cmd - common command structure
 * returns: nothing
 */

void nvmeSubmit(NVMEController *drive, int q, NVMECommonCommand *cmd) {
    NVMECommonCommand *sq;
    NVMECompletionQueue *cq;
    int tail, nextTail;

    if(!q) {
        sq = drive->asq;
        cq = drive->acq;

        tail = drive->adminTail;
        drive->adminTail++;
        if(drive->adminTail >= drive->adminQueueSize)
            drive->adminTail = 0;
        nextTail = drive->adminTail;
    } else {
        sq = drive->sq[q-1];
        cq = drive->cq[q-1];

        tail = drive->ioTails[q-1];
        drive->ioTails[q-1]++;
        if(drive->ioTails[q-1] >= drive->ioQSize)
            drive->ioTails[q-1] = 0;
        nextTail = drive->ioTails[q-1];
    }

    // copy the command into the submission queue
    memcpy(&sq[tail], cmd, sizeof(NVMECommonCommand));

    // reset the corresponding completion queue entry
    memset(&cq[tail], 0, sizeof(NVMECompletionQueue));

    // and notify the controler by updating the queue tail doorbell
    nvmeSubmitDoorbell(drive, q, nextTail);
}

/* nvmeFindQueue(): returns the least busy I/O queue of an NVMe controller
 * params: drive - NVMe controller structure
 * returns: I/O queue number (one-based because zero is the admin queue)
 */

int nvmeFindQueue(NVMEController *drive) {
    int smallest = 0xFFFFFFFF;  // arbitrarily large number that isn't allowed
    int si = 0;
    for(int i = 0; i < drive->sqCount; i++) {
        if(drive->ioBusy[i] < smallest) {
            smallest = drive->ioBusy[i];
            si = i;
        }
    }

    return si + 1;
}