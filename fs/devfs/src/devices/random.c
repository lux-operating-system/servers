/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

/* Implementation of /dev/random and /dev/urandom */

#include <stdint.h>
#include <devfs/devfs.h>
#include <liblux/liblux.h>

ssize_t randomIOHandler(int write, const char *name, off_t *position, void *buffer, size_t len) {
    uint64_t rng;
    uint8_t *ptr = buffer;
    if(!write) {
        for(int i = 0; i < len; i++) {
            luxRequestRNG(&rng);
            ptr[i] = rng >> (rng & 0xF);
            ptr[i] ^= (rng >> ((rng >> 21) & 7));
        }
    }

    *position += len;
    return len;
}
