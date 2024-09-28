/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

/* Implementation of NVMe SSD I/O based on the NVM I/O Command Set */

#include <liblux/liblux.h>
#include <nvme/nvme.h>
#include <nvme/nvmcmd.h>
#include <nvme/registers.h>
#include <string.h>

/* nvmeReadSector(): reads contiguous sectors from an NVMe SSD into memory
 * params: drive - drive to read from
 * params: id - unique non-zero command ID
 * params: ns - namespace within the drive to read from
 * params: lba - starting LBA address
 * params: count - number of sectors to read
 * params: buffer - buffer to read into
 * returns: non-zero I/O queue number on success, zero on fail
 */

int nvmeReadSector(NVMEController *drive, int ns, uint16_t id, uint64_t lba, uint16_t count, void *buffer) {
    // input validation
    if(ns >= drive->nsCount || !count || !id) return 0;
    if((lba + count) >= drive->nsSizes[ns]) return 0;

    // we will need the length of the transfer in bytes for the DMA
    size_t len = count * drive->nsSectorSizes[ns];

    // allocate a queue
    int q = nvmeFindQueue(drive);

    NVMECommonCommand cmd;
    memset(&cmd, 0, sizeof(NVMECommonCommand));
    cmd.dword0 = NVM_READ | (id << 16);
    cmd.namespaceID = drive->ns[ns];
    cmd.metaptr = 0;
    if(nvmeCreatePRP(drive, &cmd, buffer, len) < 0) return 0;

    cmd.dword2 = 0;
    cmd.dword3 = 0;
    cmd.dword10 = lba;
    cmd.dword11 = (lba >> 32);
    cmd.dword12 = count-1;
    cmd.dword13 = 0;
    cmd.dword14 = 0;
    cmd.dword15 = 0;
    nvmeSubmit(drive, q, &cmd);

    return q;
}

/* nvmeWriteSector(): writes contiguous sectors from memory to an NVMe SSD
 * params: drive - drive to write to
 * params: id - unique non-zero command ID
 * params: ns - namespace within the drive to write to
 * params: lba - starting LBA address
 * params: count - number of sectors to write
 * params: buffer - buffer to write from
 * returns: command ID on success, zero on fail
 */

int nvmeWriteSector(NVMEController *drive, int ns, uint16_t id, uint64_t lba, uint16_t count, const void *buffer) {
    // input validation
    if(ns >= drive->nsCount || !count || !id) return 0;
    if((lba + count) >= drive->nsSizes[ns]) return 0;

    // we will need the length of the transfer in bytes for the DMA
    size_t len = count * drive->nsSectorSizes[ns];

    // allocate a queue
    int q = nvmeFindQueue(drive);

    NVMECommonCommand cmd;
    memset(&cmd, 0, sizeof(NVMECommonCommand));
    cmd.dword0 = NVM_WRITE | (id << 16);
    cmd.namespaceID = drive->ns[ns];
    cmd.metaptr = 0;
    if(nvmeCreatePRP(drive, &cmd, buffer, len) < 0) return 0;

    cmd.dword2 = 0;
    cmd.dword3 = 0;
    cmd.dword10 = lba;
    cmd.dword11 = (lba >> 32);
    cmd.dword12 = count-1;
    cmd.dword13 = 0;
    cmd.dword14 = 0;
    cmd.dword15 = 0;
    nvmeSubmit(drive, q, &cmd);

    return q;
}