# Makefile for BeagleBone Black (AM335x) Bare-Metal Application
# Target: ARM Cortex-A8 (ARMv7-A)

# Cross-compiler prefix (adjust based on your toolchain)
CROSS_COMPILE ?= arm-linux-gnueabihf-

# Tools
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# Target binary name
TARGET = am335x_bare_metal

# Source files
STARTUP_SRC = startup.s
C_SOURCES = main.c
ASM_SOURCES = $(STARTUP_SRC)

# Object files
STARTUP_OBJ = $(STARTUP_SRC:.s=.o)
C_OBJECTS = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.s=.o)
OBJECTS = $(STARTUP_OBJ) $(C_OBJECTS)

# Compiler flags for bare-metal ARM Cortex-A8
CFLAGS = -march=armv7-a \
         -mtune=cortex-a8 \
         -marm \
         -mno-unaligned-access \
         -fno-builtin \
         -fno-stack-protector \
         -nostdlib \
         -nostartfiles \
         -ffreestanding \
         -O2 \
         -Wall \
         -Wextra \
         -std=c99

# Assembler flags
ASFLAGS = -march=armv7-a \
          -mtune=cortex-a8 \
          -mfloat-abi=soft

# Linker flags
LDFLAGS = -T linker.lds \
          -nostdlib \
          -nostartfiles \
          --gc-sections

# Default target
all: $(TARGET).bin $(TARGET).elf

# Create binary file for U-Boot
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	@echo "Created binary: $@"
	@echo "Ready for U-Boot loading at 0x80000000"

# Link ELF file
$(TARGET).elf: $(OBJECTS) linker.lds
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@
	@echo "Created ELF: $@"

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup file (must be first)
startup.o: startup.s
	$(CC) $(ASFLAGS) -c $< -o $@

# Generate disassembly for debugging
disasm: $(TARGET).elf
	$(OBJDUMP) -D $< > $(TARGET).dis
	@echo "Generated disassembly: $(TARGET).dis"

# Generate memory map
map: $(TARGET).elf
	$(OBJDUMP) -h $< > $(TARGET).map
	@echo "Generated memory map: $(TARGET).map"

# Display size information
size: $(TARGET).elf
	$(CROSS_COMPILE)size $<

# Debug information
debug: $(TARGET).elf
	$(OBJDUMP) -t $< | sort > $(TARGET).symbols
	@echo "Generated symbols: $(TARGET).symbols"

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET).elf $(TARGET).bin $(TARGET).dis $(TARGET).map $(TARGET).symbols

# Clean everything including backups
distclean: clean
	rm -f *~

# U-Boot loading instructions
uboot-help:
	@echo "=== U-Boot Loading Instructions ==="
	@echo "1. Copy $(TARGET).bin to SD card or load via TFTP"
	@echo "2. In U-Boot prompt:"
	@echo "   # Load binary to DDR"
	@echo "   fatload mmc 0:1 0x80000000 $(TARGET).bin"
	@echo "   # Or via TFTP:"
	@echo "   tftp 0x80000000 $(TARGET).bin"
	@echo "   # Execute"
	@echo "   go 0x80000000"
	@echo ""
	@echo "=== Memory Layout ==="
	@echo "Code Start:     0x80000000 (DDR)"
	@echo "Stack:          0x80010000 (grows down)"
	@echo "Vector Table:   0xFFFF0000 (high vectors)"
	@echo ""

# JTAG debugging help
jtag-help:
	@echo "=== JTAG Debugging ==="
	@echo "OpenOCD config for BeagleBone Black:"
	@echo "  openocd -f board/ti_beaglebone_black.cfg"
	@echo ""
	@echo "GDB commands:"
	@echo "  $(CROSS_COMPILE)gdb $(TARGET).elf"
	@echo "  (gdb) target remote localhost:3333"
	@echo "  (gdb) load"
	@echo "  (gdb) monitor reset halt"
	@echo "  (gdb) break main"
	@echo "  (gdb) continue"

# Show build info
info:
	@echo "=== Build Configuration ==="
	@echo "Cross Compiler: $(CROSS_COMPILE)gcc"
	@echo "Target:         ARM Cortex-A8 (ARMv7-A)"
	@echo "Architecture:   $(shell $(CC) -march=armv7-a -Q --help=target | grep march)"
	@echo "Linker Script:  linker.lds"
	@echo "Load Address:   0x80000000"
	@echo ""

# Help target
help:
	@echo "=== AM335x Bare-Metal Makefile ==="
	@echo "Targets:"
	@echo "  all         - Build ELF and binary files"
	@echo "  clean       - Remove object and output files"
	@echo "  disasm      - Generate disassembly listing"
	@echo "  map         - Generate memory map"
	@echo "  size        - Show section sizes"
	@echo "  debug       - Generate debug symbols"
	@echo "  info        - Show build configuration"
	@echo "  uboot-help  - Show U-Boot loading instructions"
	@echo "  jtag-help   - Show JTAG debugging setup"
	@echo "  help        - Show this help"
	@echo ""

.PHONY: all clean distclean disasm map size debug info uboot-help jtag-help help

# Dependency tracking
-include $(OBJECTS:.o=.d)