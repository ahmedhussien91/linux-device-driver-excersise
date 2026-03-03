#!/bin/bash

# deploy_test.sh - BeagleBone Black Bare-Metal Deployment Script
# This script helps deploy and test the bare-metal application

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET="am335x_bare_metal"
BINARY="${TARGET}.bin"
ELF="${TARGET}.elf"

# Configuration
BB_IP="${BB_IP:-192.168.1.100}"
TFTP_DIR="${TFTP_DIR:-/tftpboot}"
SD_MOUNT="${SD_MOUNT:-/media/$USER/BOOT}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE} BeagleBone Black Bare-Metal${NC}"
    echo -e "${BLUE} Deployment Script${NC}"
    echo -e "${BLUE}================================${NC}"
}

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_build() {
    print_status "Checking build status..."
    
    if [ ! -f "$BINARY" ]; then
        print_warning "Binary not found. Building..."
        make clean && make all
        if [ $? -ne 0 ]; then
            print_error "Build failed!"
            exit 1
        fi
    fi
    
    if [ ! -f "$ELF" ]; then
        print_error "ELF file not found!"
        exit 1
    fi
    
    # Show size information
    echo ""
    print_status "Binary size information:"
    ls -lh "$BINARY"
    arm-linux-gnueabihf-size "$ELF" 2>/dev/null || echo "Size tool not available"
    echo ""
}

deploy_tftp() {
    print_status "Deploying via TFTP..."
    
    if [ ! -d "$TFTP_DIR" ]; then
        print_error "TFTP directory not found: $TFTP_DIR"
        print_warning "Set TFTP_DIR environment variable or create the directory"
        return 1
    fi
    
    # Copy binary to TFTP directory
    sudo cp "$BINARY" "$TFTP_DIR/"
    sudo chmod 644 "$TFTP_DIR/$BINARY"
    
    print_status "Binary copied to TFTP directory: $TFTP_DIR"
    
    # Generate U-Boot commands
    cat << EOF

${GREEN}U-Boot TFTP Loading Commands:${NC}
=====================================
# Setup network (adjust IP addresses as needed)
setenv ipaddr 192.168.1.100
setenv serverip 192.168.1.1
setenv netmask 255.255.255.0

# Load binary via TFTP  
tftp 0x80000000 $BINARY

# Execute
go 0x80000000

# Or create environment variable for easier loading
setenv loadbare 'tftp 0x80000000 $BINARY; go 0x80000000'
saveenv
# Then just run: run loadbare

EOF
}

deploy_sdcard() {
    print_status "Deploying to SD card..."
    
    if [ ! -d "$SD_MOUNT" ]; then
        print_error "SD card mount point not found: $SD_MOUNT"
        print_warning "Mount the SD card or set SD_MOUNT environment variable"
        return 1
    fi
    
    # Copy binary to SD card
    cp "$BINARY" "$SD_MOUNT/"
    sync
    
    print_status "Binary copied to SD card: $SD_MOUNT"
    
    # Generate U-Boot commands
    cat << EOF

${GREEN}U-Boot SD Card Loading Commands:${NC}
======================================
# Load from SD card (FAT partition)
fatload mmc 0:1 0x80000000 $BINARY

# Execute  
go 0x80000000

# Or create environment variable
setenv loadbare 'fatload mmc 0:1 0x80000000 $BINARY; go 0x80000000'
saveenv
# Then just run: run loadbare

EOF
}

create_uboot_script() {
    print_status "Creating U-Boot script..."
    
    # Create boot script source
    cat > boot_bare_metal.cmd << EOF
echo "Loading BeagleBone Black Bare-Metal Application"
echo "Load Address: 0x80000000"

# Try TFTP first, then SD card
echo "Attempting TFTP load..."
if tftp 0x80000000 $BINARY; then
    echo "TFTP load successful"
else
    echo "TFTP failed, trying SD card..."
    if fatload mmc 0:1 0x80000000 $BINARY; then
        echo "SD card load successful"
    else
        echo "Both TFTP and SD card failed!"
        exit
    fi
fi

# Display load information
echo "Binary loaded at 0x80000000"
echo "Starting bare-metal application..."
echo "WARNING: System will not return to U-Boot!"

# Execute
go 0x80000000
EOF

    # Compile U-Boot script (if mkimage available)
    if command -v mkimage >/dev/null 2>&1; then
        mkimage -A arm -O linux -T script -C none -a 0 -e 0 \
                -n "BeagleBone Black Bare-Metal Boot" \
                -d boot_bare_metal.cmd boot_bare_metal.scr
        print_status "U-Boot script created: boot_bare_metal.scr"
        
        # Copy to deployments if requested
        if [ -d "$TFTP_DIR" ]; then
            sudo cp boot_bare_metal.scr "$TFTP_DIR/"
        fi
        
        if [ -d "$SD_MOUNT" ]; then
            cp boot_bare_metal.scr "$SD_MOUNT/"
        fi
    else
        print_warning "mkimage not found, only .cmd file created"
    fi
    
    print_status "U-Boot script source created: boot_bare_metal.cmd"
}

show_memory_map() {
    print_status "Memory Map Information:"
    cat << EOF

${YELLOW}AM335x Memory Map:${NC}
====================
0x00000000 - 0x0002BFFF : Boot ROM (176KB)
0x40200000 - 0x4020FFFF : Internal SRAM (64KB)
0x80000000 - 0x9FFFFFFF : DDR3 External RAM (512MB)

${YELLOW}Application Memory Layout:${NC}
============================
0x80000000 : Code start (load address)
0x80008000 : Data section
0x8000C000 : BSS section
0x80010000 : Stack top (grows down, 64KB)
0x80020000 : Heap (1MB)
0xFFFF0000 : Exception vectors (high)

${YELLOW}Key Peripherals:${NC}
================
0x48200000 : ARM Interrupt Controller (AINTC)
0x44E31000 : DMTimer1MS  
0x44E00400 : Clock Management (CM_WKUP)

EOF
}

show_jtag_setup() {
    print_status "JTAG Debugging Setup:"
    cat << EOF

${YELLOW}OpenOCD Setup:${NC}
==============
# Install OpenOCD with BeagleBone Black support
sudo apt install openocd

# Start OpenOCD (in separate terminal)
openocd -f board/ti_beaglebone_black.cfg

${YELLOW}GDB Debugging:${NC}
===============
# Start GDB session
arm-linux-gnueabihf-gdb $ELF

# GDB commands
(gdb) target remote localhost:3333
(gdb) load
(gdb) monitor reset halt
(gdb) break main
(gdb) break c_irq_handler
(gdb) continue

# Useful debugging commands
(gdb) info registers
(gdb) x/10i \$pc                 # Disassemble around PC
(gdb) x/16wx 0x48200040          # Examine AINTC SIR_IRQ
(gdb) x/16wx 0x44E31000          # Examine DMTimer registers

EOF
}

run_analysis() {
    print_status "Running binary analysis..."
    
    if command -v arm-linux-gnueabihf-objdump >/dev/null 2>&1; then
        echo ""
        print_status "Section headers:"
        arm-linux-gnueabihf-objdump -h "$ELF"
        
        echo ""
        print_status "Entry point and symbols:"
        arm-linux-gnueabihf-objdump -t "$ELF" | grep -E "(main|_start|irq_handler)" || true
        
        echo ""
        print_status "Disassembly of key functions:"
        arm-linux-gnueabihf-objdump -d "$ELF" | grep -A 20 "<_start>:" || true
    else
        print_warning "ARM objdump not available for analysis"
    fi
}

# Main script logic
main() {
    print_header
    
    case "${1:-help}" in
        build)
            print_status "Building bare-metal application..."
            make clean && make all
            check_build
            ;;
            
        tftp)
            check_build
            deploy_tftp
            ;;
            
        sdcard)
            check_build
            deploy_sdcard
            ;;
            
        script)
            check_build
            create_uboot_script
            ;;
            
        all)
            check_build
            deploy_tftp || true
            deploy_sdcard || true
            create_uboot_script
            ;;
            
        memory)
            show_memory_map
            ;;
            
        jtag)
            show_jtag_setup
            ;;
            
        analyze)
            check_build
            run_analysis
            ;;
            
        clean)
            print_status "Cleaning build files..."
            make clean
            rm -f boot_bare_metal.cmd boot_bare_metal.scr
            ;;
            
        help|*)
            cat << EOF
Usage: $0 [COMMAND]

Commands:
  build     Build the bare-metal application
  tftp      Deploy to TFTP directory
  sdcard    Deploy to SD card
  script    Create U-Boot loading script  
  all       Deploy via all methods
  memory    Show memory map information
  jtag      Show JTAG debugging setup
  analyze   Analyze the built binary
  clean     Clean build and deployment files
  help      Show this help

Environment Variables:
  BB_IP       BeagleBone Black IP address (default: 192.168.1.100)
  TFTP_DIR    TFTP server directory (default: /tftpboot)
  SD_MOUNT    SD card mount point (default: /media/\$USER/BOOT)

Examples:
  $0 build                    # Build the application
  $0 tftp                     # Deploy via TFTP
  $0 sdcard                   # Deploy to SD card
  BB_IP=192.168.7.2 $0 tftp   # Use different IP
  
EOF
            ;;
    esac
}

# Change to script directory
cd "$SCRIPT_DIR"

# Run main function
main "$@"