all:
	@echo "\x1B[0;1;35m make\x1B[0m servers/fs/vfs"
	@make -C vfs
	@echo "\x1B[0;1;35m make\x1B[0m servers/fs/devfs"
	@make -C devfs
	@echo "\x1B[0;1;35m make\x1B[0m servers/fs/procfs"
	@make -C procfs
	@echo "\x1B[0;1;35m make\x1B[0m servers/fs/lxfs"
	@make -C lxfs

install:
	@echo "\x1B[0;1;35m make\x1B[0m install servers/fs/vfs"
	@make install -C vfs
	@echo "\x1B[0;1;35m make\x1B[0m install servers/fs/devfs"
	@make install -C devfs
	@echo "\x1B[0;1;35m make\x1B[0m install servers/fs/procfs"
	@make install -C procfs
	@echo "\x1B[0;1;35m make\x1B[0m install servers/fs/lxfs"
	@make install -C lxfs

clean:
	@echo "\x1B[0;1;35m make\x1B[0m clean servers/fs/vfs"
	@make clean -C vfs
	@echo "\x1B[0;1;35m make\x1B[0m clean servers/fs/devfs"
	@make clean -C devfs
	@echo "\x1B[0;1;35m make\x1B[0m clean servers/fs/procfs"
	@make clean -C procfs
	@echo "\x1B[0;1;35m make\x1B[0m clean servers/fs/lxfs"
	@make clean -C lxfs