/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pty: Microkernel server implementing Unix 98-style pseudo-terminal devices
 */

/* Note to self:
 *
 * The master pseudo-terminal multiplexer is located at /dev/ptmx, and slave
 * pseudo-terminals are at /dev/ptyX. Master pseudo-terminals do not have a
 * file system representation, and they are only accessed through their file
 * descriptors. Every time an open() syscall opens /dev/ptmx, a new master-
 * slave pseudo-terminal pair is created, the file descriptor of the master is
 * returned, and the slave is created in /dev/ptyX. The master can find the
 * name of the slave using ptsname(), and the slave is deleted from the file
 * system after no more processes have an open file descriptor pointing to it.
 * 
 * After the master is created, the slave's permissions are adjusted by calling
 * grantpt() with the master file descriptor. Next, the slave is unlocked by
 * calling unlockpt() with the master file descriptor. Finally, the controlling
 * process calls ptsname() to find the slave's file name and opens it using the
 * standard open(). The opened slave file descriptor can then be set to the
 * controlling pseudo-terminal of a process using ioctl(). The input/output of
 * the slave can be read/written through the master, enabling the controlling
 * process to implement a terminal emulator.
 * 
 * https://unix.stackexchange.com/questions/405972/
 * https://unix.stackexchange.com/questions/117981/
 * https://man7.org/linux/man-pages/man7/pty.7.html
 * https://man7.org/linux/man-pages/man3/grantpt.3.html
 * https://man7.org/linux/man-pages/man3/unlockpt.3.html
 */

int main() {
    return 0;
}