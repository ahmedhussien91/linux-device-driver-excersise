# Makefile to compile each .c file as a separate program
export PATH:=${HOME}/x-tools/arm-cortex_a8-linux-gnueabihf/bin/:$(PATH)
export CROSS_COMPILE=arm-cortex_a8-linux-gnueabihf-
export ARCH=arm

# Compiler flags
CFLAGS = -Wall -Wextra -O2

# Source files
SRCS = $(wildcard *.c)

# Executable files
PROGS = $(SRCS:.c=)

# Default target
all: $(PROGS)

# Rule to build each program
%: %.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -o $@ $<
	sudo cp $@ /srv/nfs4/nfsroot_bb/root/

# Clean target
clean:
	rm $(PROGS)
