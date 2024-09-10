all:
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m servers/liblux"
	@make -C liblux
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m install servers/liblux"
	@make install -C liblux
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m servers/fs"
	@make -C fs

install:
	@mkdir -p out
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m install servers/liblux"
	@make install -C liblux
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m install servers/fs"
	@make install -C fs

clean:
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m clean servers/liblux"
	@make clean -C liblux
	@echo "\x1B[0;1;35m [  MAKE ]\x1B[0m clean servers/fs"
	@make clean -C fs
