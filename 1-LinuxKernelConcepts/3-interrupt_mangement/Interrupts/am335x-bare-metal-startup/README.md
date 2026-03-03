# AM335x BeagleBone Black Bare-Metal Startup Framework

A complete minimal bare-metal startup framework for the BeagleBone Black (AM335x Cortex-A8) that runs after U-Boot transfer.

## Overview

This framework provides a complete bare-metal environment featuring:
- ARM assembly startup code with proper exception handling
- High exception vectors (0xFFFF0000) instead of low vectors  
- ARM Interrupt Controller (AINTC) initialization
- DMTimer periodic interrupt implementation
- Cache and MMU management
- Complete interrupt handling flow
- Self-contained execution without returning to U-Boot

## Hardware Target

- **Board**: BeagleBone Black
- **SoC**: Texas Instruments AM335x (ARM Cortex-A8)
- **Architecture**: ARMv7-A  
- **Execution Mode**: Bare-metal, no Linux, no MMU
- **Memory**: 512MB DDR3 starting at 0x80000000

## Memory Map

### Physical Memory Layout

| Address Range | Size | Description |
|---------------|------|-------------|
| `0x00000000` | 176KB | Boot ROM |
| `0x40200000` | 64KB | SRAM (Internal) |
| `0x80000000` | 512MB | DDR3 External RAM |
| `0x44000000` - `0x57FFFFFF` | | Peripheral Memory Map |

### Application Memory Layout  

| Address | Description |
|---------|-------------|
| `0x80000000` | Code start (load address) |
| `0x80008000` | Data section |
| `0x8000C000` | BSS section |
| `0x80010000` | Stack top (64KB stack) |
| `0x80020000` | Heap start (1MB heap) |
| `0xFFFF0000` | High exception vectors |

### Key Peripheral Addresses

| Peripheral | Base Address | Description |
|------------|--------------|-------------|
| AINTC | `0x48200000` | ARM Interrupt Controller |
| DMTimer1MS | `0x44E31000` | 1ms Duration Timer |
| CM_WKUP | `0x44E00400` | Clock Management (Wakeup) |
| Control Module | `0x44E10000` | Pin multiplexing and control |

## Build Instructions

### Prerequisites

- ARM cross-compilation toolchain (arm-linux-gnueabihf-gcc)
- Make utility

### Building

```bash
# Build all targets
make all

# Show build configuration 
make info

# Generate disassembly (for debugging)
make disasm

# Show memory usage
make size

# Clean build files
make clean
```

### Output Files

- `am335x_bare_metal.elf` - ELF executable with debug symbols
- `am335x_bare_metal.bin` - Raw binary for U-Boot loading
- `am335x_bare_metal.dis` - Disassembly listing  
- `am335x_bare_metal.map` - Memory map

## U-Boot Loading and Execution

### Loading via FAT filesystem (SD card)

```bash
# In U-Boot prompt:
fatload mmc 0:1 0x80000000 am335x_bare_metal.bin
go 0x80000000
```

### Loading via TFTP

```bash
# Setup network (example)
setenv ipaddr 192.168.1.100
setenv serverip 192.168.1.1

# Load binary
tftp 0x80000000 am335x_bare_metal.bin
go 0x80000000
```

### Important Notes

1. **Load Address**: Always use `0x80000000` as the load address
2. **No Return**: The application does not return to U-Boot  
3. **Self-Contained**: All initialization is handled by the application
4. **High Vectors**: Uses high exception vectors at 0xFFFF0000

## Code Architecture

### Startup Sequence (startup.s)

1. **Initial State**: Execution begins in SVC mode after U-Boot
2. **Interrupt Disable**: Immediately disable IRQ and FIQ
3. **Cache Management**: Safely disable caches and MMU
4. **High Vectors Setup**: Configure SCTLR for high exception vectors
5. **Vector Table Copy**: Install custom exception vectors at 0xFFFF0000  
6. **Stack Setup**: Initialize stacks for all ARM processor modes
7. **BSS Clear**: Zero-initialize uninitialized data
8. **C Runtime**: Jump to main() function

### Exception Handling

#### Exception Vector Table

| Offset | Exception Type | Handler |
|--------|----------------|---------|
| 0x00 | Reset | `reset_handler` |
| 0x04 | Undefined Instruction | `undef_handler` |  
| 0x08 | Software Interrupt (SWI) | `swi_handler` |
| 0x0C | Prefetch Abort | `prefetch_handler` |
| 0x10 | Data Abort | `data_handler` |
| 0x14 | Reserved | - |
| 0x18 | IRQ | `irq_handler` |
| 0x1C | FIQ | `fiq_handler` |

#### IRQ Handler Flow

```assembly
irq_handler:
    sub lr, lr, #4              ; Adjust return address
    stmfd sp!, {r0-r12, lr}     ; Save context
    bl c_irq_handler            ; Call C handler
    ldmfd sp!, {r0-r12, pc}^    ; Restore context & return
```

### ARM Interrupt Controller (AINTC)

#### AINTC Configuration

1. **Reset Controller**: Soft reset via SYSCONFIG
2. **Disable Protection**: Allow register access  
3. **Priority Configuration**: Set threshold to lowest priority
4. **Mask All Interrupts**: Initially disable all interrupt sources
5. **Clear Pending**: Clear any existing pending interrupts
6. **Enable Controller**: Enable new IRQ/FIQ generation

#### Interrupt Handling Flow

1. **IRQ Reception**: CPU receives IRQ signal
2. **Context Save**: Assembly handler saves registers
3. **Read SIR_IRQ**: Get active interrupt number
4. **Peripheral Handling**: Clear peripheral interrupt source
5. **AINTC Acknowledge**: Signal AINTC that IRQ is handled
6. **Context Restore**: Restore registers and return

#### Key Registers

| Register | Offset | Purpose |
|----------|---------|---------|  
| SYSCONFIG | 0x10 | Reset and idle configuration |
| SIR_IRQ | 0x40 | Active IRQ number |
| CONTROL | 0x48 | Controller enable and acknowledge |
| MIR_CLEAR(n) | 0x88 + n*0x20 | Unmask interrupts |
| MIR_SET(n) | 0x8C + n*0x20 | Mask interrupts |

### DMTimer Configuration

#### DMTimer1MS Setup

1. **Clock Enable**: Enable DMTimer1 clock via CM_WKUP
2. **Reset Timer**: Software reset via TIOCP_CFG
3. **Configure Period**: Set TLDR for 1ms period (24MHz clock assumed)
4. **Enable Interrupt**: Enable overflow interrupt via TIER
5. **Start Timer**: Enable auto-reload and start via TCLR

#### Timer Calculation

For 1ms period with 24MHz clock:
```
Timer Period = Clock Period × Timer Count
1ms = (1/24MHz) × Timer_Count
Timer_Count = 24000 ticks

Reload Value = 0xFFFFFFFF - 24000 + 1 = 0xFFFFA240
```

## System Control Register (SCTLR) Configuration

### High Vector Setup

```c
mrc p15, 0, r0, c1, c0, 0   // Read SCTLR
orr r0, r0, #(1 << 13)      // Set V bit (high vectors)  
bic r0, r0, #(1 << 0)       // Clear M bit (disable MMU)
bic r0, r0, #(1 << 2)       // Clear C bit (disable D-cache)
bic r0, r0, #(1 << 12)      // Clear I bit (disable I-cache)
mcr p15, 0, r0, c1, c0, 0   // Write SCTLR
```

### SCTLR Bit Definitions

| Bit | Name | Function |
|-----|------|----------|
| 0 | M | MMU Enable |
| 2 | C | Data Cache Enable |
| 12 | I | Instruction Cache Enable |
| 13 | V | Exception Vector Location (0=Low, 1=High) |

## Cache and Memory Management

### Cache Invalidation Sequence

1. **Disable Caches**: Clear C and I bits in SCTLR
2. **Instruction Cache**: Invalidate all via ICIALLU
3. **Data Cache**: Invalidate all via DCISW  
4. **TLB Invalidation**: Invalidate all TLBs via TLBIALL
5. **Barriers**: Use DSB and ISB for ordering

### Memory Barriers

- **DMB**: Data Memory Barrier - ensures memory accesses complete
- **DSB**: Data Synchronization Barrier - stronger ordering
- **ISB**: Instruction Synchronization Barrier - flush pipeline

## Stack Configuration

### Mode-Specific Stacks

| Mode | Stack Size | Location |
|------|------------|----------|
| SVC (Supervisor) | Main stack | 0x80010000 (top) |
| IRQ | 4KB | 0x8000F000 |
| FIQ | 4KB | 0x8000E000 |  
| UND (Undefined) | 4KB | 0x8000D000 |
| ABT (Abort) | 4KB | 0x8000C000 |

## Debugging

### JTAG Debugging with OpenOCD

```bash
# Start OpenOCD
openocd -f board/ti_beaglebone_black.cfg

# In separate terminal, start GDB  
arm-linux-gnueabihf-gdb am335x_bare_metal.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) monitor reset halt
(gdb) break main
(gdb) continue
```

### Debug Outputs

The framework generates several debug files:
- **Disassembly**: `make disasm` creates complete disassembly
- **Memory Map**: `make map` shows section layout
- **Symbols**: `make debug` creates sorted symbol table

## Tested Functionality

### Verified Features

✅ U-Boot loading at 0x80000000  
✅ Safe cache and MMU disable  
✅ High vector installation  
✅ AINTC initialization  
✅ DMTimer periodic interrupts  
✅ Proper interrupt acknowledge flow  
✅ Context save/restore in IRQ handler  

### Interrupt Flow Verification

1. DMTimer generates overflow interrupt
2. CPU receives IRQ signal  
3. High vector IRQ handler executes
4. Context saved to SVC stack
5. C handler reads SIR_IRQ register  
6. Timer interrupt flag cleared
7. AINTC acknowledged via CONTROL register
8. Context restored and execution resumed

## Expansion Possibilities

### Additional Peripherals

- **GPIO**: LED blinking, button input
- **UART**: Serial communication for debugging  
- **I2C**: Sensor communication
- **SPI**: External device communication
- **ADC**: Analog input reading

### Advanced Features

- **FIQ Handler**: Fast interrupt processing
- **Multiple Timers**: Various timer sources
- **DMA**: Direct Memory Access configuration
- **Power Management**: Clock gating, power domains

## Troubleshooting

### Common Issues

1. **No Interrupts**: Check AINTC configuration and interrupt unmask
2. **System Hang**: Verify stack setup and exception vectors
3. **Timer Not Working**: Confirm clock enable and timer configuration  
4. **Cache Coherency**: Ensure proper memory barriers

### Debug Techniques

- Monitor SIR_IRQ register to verify interrupt delivery
- Check PENDING_IRQ registers for pending interrupts
- Verify MIR registers for interrupt mask status  
- Use JTAG to single-step through interrupt handlers

## Technical References

- **AM335x Technical Reference Manual**: TI SPRUH73P
- **ARM Architectural Reference Manual**: ARMv7-A/R Edition  
- **Cortex-A8 Technical Reference Manual**: ARM DDI 0344K
- **U-Boot Documentation**: Boot sequence and loading

## License

This framework is provided as educational material for bare-metal ARM development. Use and modify as needed for your projects.