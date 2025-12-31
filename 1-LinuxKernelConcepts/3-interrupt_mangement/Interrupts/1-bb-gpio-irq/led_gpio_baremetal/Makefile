CFLAGS := -ffreestanding -nostdlib -Wall -Wextra
LDFLAGS := -T linker.ld -nostdlib

all: baremetal.elf baremetal.bin

startup.o: startup.S
	$(AS) -c $< -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

baremetal.elf: startup.o main.o
	$(LD) $^ -o $@ $(LDFLAGS)

baremetal.bin: baremetal.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin
