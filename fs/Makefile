all:
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m servers/fs/vfs"
	@make -C vfs
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m servers/fs/devfs"
	@make -C devfs

install:
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m install servers/fs/vfs"
	@make install -C vfs
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m install servers/fs/devfs"
	@make install -C devfs

clean:
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m clean servers/fs/vfs"
	@make clean -C vfs
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m clean servers/fs/devfs"
	@make clean -C devfs
