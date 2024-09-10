all:
	@mkdir -p out
	@make -C fs

install:
	@make install -C fs

clean:
	@make clean -C fs
