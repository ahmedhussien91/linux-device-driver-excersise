CROSS_COMPILE := arm-poky-linux-gnueabi-
AS := $(CROSS_COMPILE)as
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

CFLAGS := -ffreestanding -nostdlib -Wall -Wextra -fno-stack-protector -O1 -mcpu=cortex-a8 -marm
LDFLAGS := -T linker.ld -nostdlib -Wl,-Map=baremetal.map
ASFLAGS := -march=armv7-a

all: baremetal.elf baremetal.bin

startup.o: startup.S
	$(AS) $(ASFLAGS) -c $< -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

uart.o: uart.c
	$(CC) $(CFLAGS) -c $< -o $@

baremetal.elf: startup.o main.o uart.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

baremetal.bin: baremetal.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin
