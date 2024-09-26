/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

/* NVM Express Hardware Registers */

/* The registers at offsets smaller than 0x1000 are generic to all NVMe
 * controllers, while the doorbell registers at offset 0x1000 and greater are
 * specific to NVMe over PCIe transport, which is the device that this driver
 * implements support for.
 */

#pragma once

#include <sys/types.h>
#include <nvme/nvme.h>

#define NVME_CAP            0x00        // 64-bit, controller capabilities
#define NVME_VERSION        0x08        // 32-bit
#define NVME_INT_MASK       0x0C        // 32-bit
#define NVME_INT_UNMASK     0x10        // 32-bit
#define NVME_CONFIG         0x14        // 32-bit
#define NVME_STATUS         0x1C        // 32-bit
#define NVME_RESET          0x20        // 32-bit
#define NVME_AQA            0x24        // 32-bit
#define NVME_ASQ            0x28        // 64-bit, admin submission queue
#define NVME_ACQ            0x30        // 64-bit, admin completion queue
#define NVME_BUFFER         0x38        // 32-bit, controller memory buffer
#define NVME_BUFFER_SIZE    0x3C        // 32-bit
#define NVME_BOOT_INFO      0x40        // 32-bit
#define NVME_BOOT_READ_SEL  0x44        // 32-bit
#define NVME_BOOT_BUFFER    0x48        // 64-bit
#define NVME_BUFFER_CONTROL 0x50        // 64-bit
#define NVME_BUFFER_STATUS  0x58        // 32-bit
#define NVME_BUFFER_ELAS    0x5C        // 32-bit
#define NVME_BUFFER_WTP     0x60        // 32-bit
#define NVME_SHUTDOWN       0x64        // 32-bit
#define NVME_TIMEOUTS       0x68        // 32-bit
#define NVME_DOORBELLS      0x1000      // 32-bit, base of doorbell registers

/* NVMe Capability Register: NVME_CAP */
#define NVME_CAP_MAXQ_MASK      0xFFFF          // maximum queues
#define NVME_CAP_CONT_QUEUE     0x10000         // if the controller requires physically contiguous queues
#define NVME_CAP_TIMEOUT_MASK   0xF0000000
#define NVME_CAP_TIMEOUT_SHIFT  24
#define NVME_CAP_DSTRD_MASK     0xF00000000L    // doorbell stride
#define NVME_CAP_DSTRD_SHIFT    32
#define NVME_CAP_RESET          0x1000000000L
#define NVME_CAP_NVM_CMDS       0x2000000000L   // controller supports the NVM command set
#define NVME_CAP_IO_CMDS        0x80000000000L  // controller supports one or more I/O command sets
#define NVME_CAP_NO_IO_CMDS     0x100000000000L // controller doesn't support I/O command sets
#define NVME_CAP_BOOT           0x200000000000L
#define NVME_CAP_MPSMIN_MASK    0xF000000000000L
#define NVME_CAP_MPSMIN_SHIFT   48
#define NVME_CAP_MPSMAX_MASK    0xF0000000000000L
#define NVME_CAP_MPSMAX_SHIFT   52
#define NVME_CAP_SHUTDOWN       0x400000000000000L
#define NVME_CAP_SHUTDOWN_ENC   0x2000000000000000L

/* NVMe Controller Configuration Register: NVME_CONFIG */
#define NVME_CONFIG_EN          0x01            // enable/disable the controller
#define NVME_CONFIG_CMDS_NVM    0x00            // command set selector
#define NVME_CONFIG_CMDS_ALL    0x60
#define NVME_CONFIG_CMDS_ADMIN  0x70
#define NVME_CONFIG_CMDS_MASK   0x70
#define NVME_CONFIG_CMDS_SHIFT  4
#define NVME_CONFIG_MPS_MASK    0x0F
#define NVME_CONFIG_MPS_SHIFT   7
#define NVME_CONFIG_SQES_MASK   0x0F            // submission queue entry size
#define NVME_CONFIG_SQES_SHIFT  16
#define NVME_CONFIG_CQES_MASK   0x0F            // completion queue entry size
#define NVME_CONFIG_CQES_SHIFT  20

/* NVMe Controller Status Register: NVME_STATUS */
#define NVME_STATUS_RDY         0x01            // ready to process queues
#define NVME_STATUS_FATAL       0x02            // unrecoverable error flag
#define NVME_STATUS_RESET_ERROR 0x10
#define NVME_STATUS_PAUSED      0x20            // only valid when STATUS.RDY==1 and CONFIG.EN==1
#define NVME_STATUS_SHUTDOWN    0x40

/* NVMe Reset Magic Number */
#define NVME_RESET_MAGIC        0x4E564D65

/* NVMe Doorbell Registers Addresses:
 *
 * 0x1000 + (2y * (4 << CAP.DSTRD))     Submission queue y tail doorbell
 * 0x1000 + ((2y+1) * (4 << CAP>DSTRD)) Completion queue y head doorbell
 */

uint32_t nvmeRead32(NVMEController *, off_t);
uint64_t nvmeRead64(NVMEController *, off_t);
void nvmeWrite32(NVMEController *, off_t, uint32_t);
void nvmeWrite64(NVMEController *, off_t, uint64_t);
