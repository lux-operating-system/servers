PLATFORM=x86_64-lux
CCFLAGS=-Wall -c -I./src/include -O3
LDFLAGS=-llux
CC=x86_64-lux-gcc
LD=x86_64-lux-gcc
SRC:=$(shell find ./src -type f -name "*.c")
OBJ:=$(SRC:.c=.o)

all: kthd

%.o: %.c
	@echo "\x1B[0;1;32m cc  \x1B[0m $<"
	@$(CC) $(CCFLAGS) -o $@ $<

kthd: $(OBJ)
	@echo "\x1B[0;1;93m ld  \x1B[0m kthd"
	@$(LD) $(OBJ) -o kthd $(LDFLAGS)

install: kthd
	@cp kthd ../out/

clean:
	@rm -f kthd $(OBJ)
