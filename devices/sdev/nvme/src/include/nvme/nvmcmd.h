/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

/* NVMe Admin and NVM I/O Command Set Opcode Declarations */

#pragma once

/* Data Flow Direction Flags of the Opcode */
#define NVM_HOST_TO_CONTROLLER      0x01
#define NVM_CONTROLLER_TO_HOST      0x02
#define NVM_BIDIRECTIONAL           0x03

/* NVMe Admin Command Set */
/* These commands are submitted to the admin queue, and their completion and/or
 * errors will reflect on the admin completion queue */

#define NVME_ADMIN_DELETE_SUBQ      0x00
#define NVME_ADMIN_CREATE_SUBQ      0x01
#define NVME_ADMIN_GET_LOG          0x02
#define NVME_ADMIN_DELETE_COMQ      0x04
#define NVME_ADMIN_CREATE_COMQ      0x05
#define NVME_ADMIN_IDENTIFY         0x06
#define NVME_ADMIN_ABORT            0x08
#define NVME_ADMIN_SET_FEATURES     0x09
#define NVME_ADMIN_GET_FEATURES     0x0A
#define NVME_ADMIN_ASYNC_REQUEST    0x0C
#define NVME_ADMIN_SELF_TEST        0x14

/* NVM Command Set */
/* These commands are submitted to the I/O submission queues followed by
 * updating the corresponding queue doorbell, and their completion and/or
 * errors will likewise update the corresponding I/O completion queue, followed
 * by a software update of the corresponding completion doorbell */

#define NVM_FLUSH                   0x00
#define NVM_WRITE                   0x01
#define NVM_READ                    0x02
#define NVM_WRITE_UNCORRECTABLE     0x04
#define NVM_COMPARE                 0x05
#define NVM_WRITE_ZERO              0x08
#define NVM_DATASET_MGMT            0x09
#define NVM_VERIFY                  0x0C
#define NVM_CANCEL                  0x18
#define NVM_COPY                    0x19
#define NVM_IO_MGMT                 0x1D
