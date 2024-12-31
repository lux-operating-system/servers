/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#pragma once

#include <sys/types.h>
#include <liblux/liblux.h>

/* with a block size of 2 KB, this will give us 8 MB of cache */
#define CACHE_SIZE          4096

typedef struct {
    int valid, dirty;
    uint64_t tag;
    void *data;
} Cache;

typedef struct Mountpoint {
    struct Mountpoint *next;
    char device[MAX_FILE_PATH];
    int fd;
    int sectorSize, blockSize, blockSizeBytes;

    uint64_t volumeSize;        // in blocks
    uint64_t root;              // root directory block
    void *blockTableBuffer;     // of size blockSizeBytes
    void *dataBuffer;           // of size 2 * blockSizeBytes
    void *meta;                 // metadata buffer, blockSizeBytes

    Cache *cache;
} Mountpoint;

typedef struct {
    uint8_t bootCode1[4];
    uint32_t identifier;
    uint64_t volumeSize;
    uint64_t rootBlock;
    uint8_t parameters;
    uint8_t version;
    uint8_t name[16];
    uint8_t reserved[6];

    // more boot code follows
} __attribute__((packed)) LXFSIdentification;

#define LXFS_MAGIC                  0x5346584C  // 'LXFS', little endian
#define LXFS_VERSION                0x01

#define LXFS_ID_BOOTABLE            0x01
#define LXFS_ID_SECTOR_SIZE_SHIFT   1
#define LXFS_ID_SECTOR_SIZE_MASK    0x03
#define LXFS_ID_BLOCK_SIZE_SHIFT    3
#define LXFS_ID_BLOCK_SIZE_MASK     0x0F

typedef struct {
    uint32_t identifier;
    uint32_t cpuArch;
    uint64_t timestamp;
    uint8_t description[32];
    uint8_t reserved[16];
} __attribute__((packed)) LXFSBootHeader;

#define LXFS_CPU_X86                0x00000001
#define LXFS_CPU_X86_64             0x00000002
#define LXFS_CPU_MIPS32             0x00000003
#define LXFS_CPU_MIPS64             0x00000004

#define LXFS_BLOCK_FREE             0x0000000000000000
#define LXFS_BLOCK_ID               0xFFFFFFFFFFFFFFFC
#define LXFS_BLOCK_BOOT             0xFFFFFFFFFFFFFFFD
#define LXFS_BLOCK_TABLE            0xFFFFFFFFFFFFFFFE
#define LXFS_BLOCK_EOF              0xFFFFFFFFFFFFFFFF

typedef struct {
    uint64_t createTime;
    uint64_t modTime;
    uint64_t accessTime;
    uint64_t sizeEntries;
    uint64_t sizeBytes;
    uint64_t reserved;
} __attribute__((packed)) LXFSDirectoryHeader;

typedef struct {
    uint16_t flags;

    uint16_t owner;
    uint16_t group;
    uint16_t permissions;
    uint64_t size;

    uint64_t createTime;
    uint64_t modTime;
    uint64_t accessTime;

    uint64_t block;
    uint16_t entrySize;
    uint8_t reserved[14];
    uint8_t name[512];
} __attribute__((packed)) LXFSDirectoryEntry;

#define LXFS_DIR_VALID              0x0001
#define LXFS_DIR_TYPE_SHIFT         1
#define LXFS_DIR_TYPE_MASK          0x03
#define LXFS_DIR_DELETED            0x1000

#define LXFS_DIR_TYPE_FILE          0x00
#define LXFS_DIR_TYPE_DIR           0x01
#define LXFS_DIR_TYPE_SOFT_LINK     0x02
#define LXFS_DIR_TYPE_HARD_LINK     0x03

#define LXFS_PERMS_OWNER_R          0x0001
#define LXFS_PERMS_OWNER_W          0x0002
#define LXFS_PERMS_OWNER_X          0x0004
#define LXFS_PERMS_GROUP_R          0x0008
#define LXFS_PERMS_GROUP_W          0x0010
#define LXFS_PERMS_GROUP_X          0x0020
#define LXFS_PERMS_OTHER_R          0x0040
#define LXFS_PERMS_OTHER_W          0x0080
#define LXFS_PERMS_OTHER_X          0x0100

typedef struct {
    uint64_t size;
    uint64_t refCount;
} __attribute__((packed)) LXFSFileHeader;

void lxfsMount(MountCommand *);
int lxfsFlushBlock(Mountpoint *, uint64_t);
int lxfsReadBlock(Mountpoint *, uint64_t, void *);
int lxfsWriteBlock(Mountpoint *, uint64_t, const void *);
uint64_t lxfsNextBlock(Mountpoint *, uint64_t);
uint64_t lxfsReadNextBlock(Mountpoint *, uint64_t, void *);
uint64_t lxfsWriteNextBlock(Mountpoint *, uint64_t, const void *);
uint64_t lxfsFindFreeBlock(Mountpoint *, uint64_t);
int lxfsSetNextBlock(Mountpoint *, uint64_t, uint64_t);

Mountpoint *findMP(const char *);
int pathDepth(const char *);
char *pathComponent(char *, const char *, int);
LXFSDirectoryEntry *lxfsFind(LXFSDirectoryEntry *, Mountpoint *, const char *);
int lxfsCreate(LXFSDirectoryEntry *, Mountpoint *, const char *, mode_t, uid_t, gid_t);

void lxfsOpen(OpenCommand *);
void lxfsStat(StatCommand *);
void lxfsRead(RWCommand *);
void lxfsWrite(RWCommand *);
void lxfsOpendir(OpendirCommand *);
void lxfsReaddir(ReaddirCommand *);
void lxfsMmap(MmapCommand *);
