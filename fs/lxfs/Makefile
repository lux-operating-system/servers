PLATFORM=x86_64-lux
CCFLAGS=-Wall -c -I./src/include -I../common/include -O3
LDFLAGS=-llux
CC=x86_64-lux-gcc
LD=x86_64-lux-gcc
SRC:=$(shell find ./src -type f -name "*.c")
OBJ:=$(SRC:.c=.o)

all: lxfs

%.o: %.c
	@echo "\x1B[0;1;32m cc  \x1B[0m $<"
	@$(CC) $(CCFLAGS) -o $@ $<

lxfs: $(OBJ)
	@echo "\x1B[0;1;93m ld  \x1B[0m lxfs"
	@$(LD) $(OBJ) -o lxfs $(LDFLAGS)

install: lxfs
	@cp lxfs ../../out/

clean:
	@rm -f lxfs $(OBJ)
