all:
	@echo "\x1B[0;1;35m make\x1B[0m servers/devices/sdev/nvme"
	@make -C nvme
	@echo "\x1B[0;1;35m make\x1B[0m servers/devices/sdev/sdev"
	@make -C sdev

install:
	@echo "\x1B[0;1;35m make\x1B[0m install servers/devices/nvme"
	@make install -C nvme
	@echo "\x1B[0;1;35m make\x1B[0m install servers/devices/sdev"
	@make install -C sdev

clean:
	@echo "\x1B[0;1;35m make\x1B[0m clean servers/devices/nvme"
	@make clean -C nvme
	@echo "\x1B[0;1;35m make\x1B[0m clean servers/devices/sdev"
	@make clean -C sdev