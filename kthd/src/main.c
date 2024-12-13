/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * kthd: Kernel Thread Helper Daemon
 */

#include <liblux/liblux.h>
#include <stdlib.h>
#include <unistd.h>

void kthdExec(ExecCommand *);
void kthdChdir(ChdirCommand *);

int main() {
    luxInit("kthd");

    SyscallHeader *msg = calloc(1, sizeof(SyscallHeader));
    if(!msg) {
        luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory for message handling\n");
        return -1;
    }

    // notify lumen that we're ready
    // there isn't anything to do until we receive requests
    luxReady();

    for(;;) {
        // receive requests from lumen
        ssize_t s = luxRecvLumen(msg, SERVER_MAX_SIZE, false, true);
        if(s > 0 && s <= SERVER_MAX_SIZE) {
            if(msg->header.length > SERVER_MAX_SIZE) {
                void *newptr = realloc(msg, msg->header.length);
                if(!newptr) {
                    luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory for message handling\n");
                    return -1;
                }

                msg = newptr;
            }

            luxRecvLumen(msg, SERVER_MAX_SIZE, false, false);

            switch(msg->header.command) {
            case COMMAND_EXEC: kthdExec((ExecCommand *) msg); break;
            case COMMAND_CHDIR: kthdChdir((ChdirCommand *) msg); break;
            default:
                luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%04X, dropping message...\n", msg->header.command);
            }
        } else {
            sched_yield();
        }
    }
}