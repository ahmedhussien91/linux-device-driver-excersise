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
# Clean target
clean:
	rm $(PROGS)
