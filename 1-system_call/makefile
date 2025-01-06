# Makefile to compile each .c file as a separate program
export PATH:=${HOME}/x-tools/arm-cortex_a8-linux-gnueabihf/bin/:$(PATH)
export CROSS_COMPILE=arm-cortex_a8-linux-gnueabihf-
export ARCH=arm

# Compiler flags
CFLAGS = -Wall -Wextra -O2

# Source files
SRCS = $(wildcard *.s)

# Executable files
PROGS = $(SRCS:.s=)

# Default target
all: $(PROGS)

# Rule to build each program
%: %.s
	$(CROSS_COMPILE)gcc $(CFLAGS) -o $@ $< -nostartfiles
	sudo cp $@ /srv/nfs4/nfsroot_bb/root/

# Clean target
clean:
	rm $(PROGS)
