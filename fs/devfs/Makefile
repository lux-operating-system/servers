PLATFORM=x86_64-lux
CCFLAGS=-c -I./src/include -I../common/include -O3
LDFLAGS=-llux
CC=x86_64-lux-gcc
LD=x86_64-lux-gcc
SRC:=$(shell find ./src -type f -name "*.c")
OBJ:=$(SRC:.c=.o)

all: devfs

%.o: %.c
	@echo "\x1B[0;1;32m [  CC   ]\x1B[0m $<"
	@$(CC) $(CCFLAGS) -o $@ $<

devfs: $(OBJ)
	@echo "\x1B[0;1;34m [  LD   ]\x1B[0m devfs"
	@$(LD) $(OBJ) -o devfs $(LDFLAGS)

install: devfs
	@cp devfs ../../out/

clean:
	@rm -f devfs $(OBJ)
