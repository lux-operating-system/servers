<div align="center">

[![luxOS logo](https://jewelcodes.io/lux/logo-small.png)](https://github.com/lux-operating-system)

[![License: MIT](https://img.shields.io/github/license/lux-operating-system/servers?color=red)](https://github.com/lux-operating-system/servers/blob/main/LICENSE) [![GitHub commit activity](https://img.shields.io/github/commit-activity/m/lux-operating-system/servers)](https://github.com/lux-operating-system/servers/commits/main/) [![Build Status](https://github.com/lux-operating-system/servers/actions/workflows/build-mac.yml/badge.svg)](https://github.com/lux-operating-system/servers/actions) [![GitHub Issues](https://img.shields.io/github/issues/lux-operating-system/servers)](https://github.com/lux-operating-system/servers/issues) [![Codacy Badge](https://app.codacy.com/project/badge/Grade/d525486004314ee4911b888b54058ced)](https://app.codacy.com/gh/lux-operating-system/servers/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

#

</div>

This repository contains the microkernel servers that implement drivers and external system call handlers for the [lux microkernel](https://github.com/lux-operating-system/kernel). The microkernel and the servers, along with [lumen](https://github.com/lux-operating-system/lumen), form the foundation of [luxOS](https://github.com/lux-operating-system/lux).

# Overview

In brief, the lux microkernel provides minimal kernel-level services and relays almost all its system calls to external servers to be handled in user space. The only OS functionality implemented at the kernel level are memory management, scheduling, and interprocess communication. The microkernel, along with these servers, all depend on [lumen](https://github.com/lux-operating-system/lumen), a user space router and [init program](https://en.wikipedia.org/wiki/Init) that facilitates communication between the kernel and the appropriate servers.

# Contents

The servers are broadly categorized into two main categories, `devices` for device drivers, and `fs` for file system drivers. A small library containing common message structures and helper functions that are repetitively used in all the servers, as well as in lumen, is also implemented in `liblux`.

Below is an alphabetically sorted list of the servers implemented thus far, their purpose, and their server dependency, if any. Any server can have at most one direct dependency. Server dependencies are inherited, establishing a clear chain of command and line of communication between the servers, the microkernel, and lumen. For this reason, the server dependency relationships may also be visualized in a tree-like structure.

| Server | Dependency | Purpose |
| ------ | ---------- | ------- |
| [devfs](https://github.com/lux-operating-system/servers/tree/main/fs/devfs) | vfs | Implementation of the `/dev` file system |
| [ide](https://github.com/lux-operating-system/servers/tree/main/devices/sdev/ide) | sdev | Device driver for IDE storage (ATA HDDs) |
| [kbd](https://github.com/lux-operating-system/servers/tree/main/devices/kbd) | devfs | Generic keyboard device interface `/dev/kbd` |
| [lfb](https://github.com/lux-operating-system/servers/tree/main/devices/lfb) | devfs | Generic linear frame buffer interface `/dev/lfbX` |
| [lxfs](https://github.com/lux-operating-system/servers/tree/main/fs/lxfs) | vfs | File system driver for [lxfs](https://github.com/lux-operating-system/lxfs), a custom 64-bit FAT-like file system with native support for Unix file permissions, long file names, and Unicode |
| [nvme](https://github.com/lux-operating-system/servers/tree/main/devices/sdev/nvme) | sdev | Device driver for NVMe SSDs |
| [pci](https://github.com/lux-operating-system/servers/tree/main/devices/pci) | devfs | Device driver for the PCI bus |
| [procfs](https://github.com/lux-operating-system/servers/tree/main/fs/procfs) | vfs | Implementation of the `/proc` file system |
| [ps2](https://github.com/lux-operating-system/servers/tree/main/devices/ps2) | kbd | Device driver for PS/2 keyboards |
| [pty](https://github.com/lux-operating-system/servers/tree/main/devices/pty) | devfs | Driver for Unix-style pseudoterminal devices `/dev/ptmx` and `/dev/ptsX` |
| [sdev](https://github.com/lux-operating-system/servers/tree/main/devices/sdev/sdev) | devfs | Generic storage device interface `/dev/sdX` |
| [vfs](https://github.com/lux-operating-system/servers/tree/main/fs/vfs) | None | Implementation of a Unix-like virtual file system |

# License

The lux microkernel and its servers are free and open source software released under the terms of the MIT License.

#

Made with 💗 from Boston and Cairo
