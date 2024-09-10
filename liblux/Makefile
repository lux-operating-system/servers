PLATFORM=x86_64-lux
CCFLAGS=-c -I./src/include -O3
LDFLAGS=
CC=x86_64-lux-gcc
AR=x86_64-lux-ar
SRC:=$(shell find ./src -type f -name "*.c")
OBJ:=$(SRC:.c=.o)
ROOT:=$(shell whereis -bq x86_64-lux-gcc | sed 's/\/bin\/x86_64-lux-gcc//g')

all: liblux.a

%.o: %.c
	@echo "\x1B[0;1;32m [  CC   ]\x1B[0m $<"
	@$(CC) $(CCFLAGS) -o $@ $<

liblux.a: $(OBJ)
	@echo "\x1B[0;1;34m [  AR   ]\x1B[0m liblux.a"
	@$(AR) rvs liblux.a $(OBJ)

install: liblux.a
	@echo "liblux: install binaries in $(ROOT)/lib"
	@cp liblux.a $(ROOT)/lib/
	@echo "liblux: install binaries in $(ROOT)/x86_64-lux/lib"
	@cp liblux.a $(ROOT)/lib/
	@echo "liblux: install binaries in $(ROOT)/lib/gcc/x86_64-lux/14.2.0/"
	@cp liblux.a $(ROOT)/lib/gcc/x86_64-lux/14.2.0/
	@echo "liblux: install headers in $(ROOT)/lib/gcc/x86_64-lux/14.2.0/include/"
	@cp -R ./src/include/* $(ROOT)/lib/gcc/x86_64-lux/14.2.0/include/

clean:
	@rm -f liblux.a $(OBJ)
