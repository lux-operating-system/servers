PLATFORM=x86_64-lux
CCFLAGS=-Wall -c -I./src/include -I../common/include -O3
LDFLAGS=-llux
CC=x86_64-lux-gcc
LD=x86_64-lux-gcc
SRC:=$(shell find ./src -type f -name "*.c")
OBJ:=$(SRC:.c=.o)

all: vfs

%.o: %.c
	@echo "\x1B[0;1;32m cc  \x1B[0m $<"
	@$(CC) $(CCFLAGS) -o $@ $<

vfs: $(OBJ)
	@echo "\x1B[0;1;34m ld  \x1B[0m vfs"
	@$(LD) $(OBJ) -o vfs $(LDFLAGS)

install: vfs
	@cp vfs ../../out/

clean:
	@rm -f vfs $(OBJ)
