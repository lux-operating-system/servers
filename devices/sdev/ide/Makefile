PLATFORM=x86_64-lux
CCFLAGS=-Wall -c -I./src/include -O3
LDFLAGS=-llux
CC=x86_64-lux-gcc
LD=x86_64-lux-gcc
SRC:=$(shell find ./src -type f -name "*.c")
OBJ:=$(SRC:.c=.o)

all: ide

%.o: %.c
	@echo "\x1B[0;1;32m cc  \x1B[0m $<"
	@$(CC) $(CCFLAGS) -o $@ $<

ide: $(OBJ)
	@echo "\x1B[0;1;93m ld  \x1B[0m ide"
	@$(LD) $(OBJ) -o ide $(LDFLAGS)

install: ide
	@cp ide ../../../out/

clean:
	@rm -f ide $(OBJ)
