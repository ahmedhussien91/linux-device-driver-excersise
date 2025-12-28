#!/bin/bash

# BeagleBone Black Network Boot Deployment Script
# Deploys kernel, DTB, and modules to TFTP and NFS for network boot
# Usage: ./deploy_network_boot.sh [build|deploy|all]

set -e

# Check if running as root/sudo - this script should not be run with sudo
if [ "$EUID" -eq 0 ]; then
    echo "❌ Do not run this script with sudo!"
    echo "The script will request sudo only when needed for file operations."
    echo "Usage: ./deploy_network_boot.sh deploy-direct"
    exit 1
fi

# Configuration
YOCTO_BASE_DIR="/opt/yocto/ycoto-excersise"
YOCTO_BUILD_DIR="/opt/yocto/tmp/work/beaglebone-poky-linux-gnueabi/linux-bb.org/6.6.32+git"
KERNEL_BUILD_DIR="$YOCTO_BUILD_DIR/linux-bb.org-6.6.32+git"
TFTP_DIR="/srv/tftp"
NFS_ROOT="/srv/nfs4/bb_busybox"
BB_IP="192.168.1.107"
SERVER_IP="192.168.1.11"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_prerequisites() {
    print_status "Checking prerequisites..."
    
    # Check if sudo is available (but don't require running as root)
    if ! sudo -n true 2>/dev/null; then
        print_status "Testing sudo access..."
        if ! sudo true; then
            print_error "This script needs sudo access to write to TFTP and NFS directories"
            exit 1
        fi
    fi
    
    # Check TFTP directory
    if [ ! -d "$TFTP_DIR" ]; then
        print_error "TFTP directory $TFTP_DIR not found"
        exit 1
    fi
    
    # Check NFS root directory
    if [ ! -d "$NFS_ROOT" ]; then
        print_error "NFS root directory $NFS_ROOT not found"
        exit 1
    fi
    
    # Check Yocto build directory
    if [ ! -d "$YOCTO_BUILD_DIR" ]; then
        print_error "Yocto build directory $YOCTO_BUILD_DIR not found"
        print_error "Please build the kernel first: bitbake virtual/kernel"
        exit 1
    fi
    
    print_success "Prerequisites check passed"
}

build_kernel_devtool() {
    print_status "Building kernel using devtool..."
    
    # Navigate to Yocto directory
    if [ ! -d "$YOCTO_BASE_DIR" ]; then
        print_error "Yocto directory $YOCTO_BASE_DIR not found"
        exit 1
    fi
    
    print_status "Please run the following commands manually:"
    echo ""
    echo "cd $YOCTO_BASE_DIR"
    echo "./bitbake_bb_rpi -h bb -d sysv -s"
    echo ""
    echo "# Then in the sourced environment:"
    echo "devtool modify linux-bb.org    # (if not already done - creates workspace)"
    echo "devtool build linux-bb.org     # (builds kernel to $YOCTO_BUILD_DIR)"
    echo ""
    print_status "Build output will be located at:"
    echo "  Kernel: $KERNEL_BUILD_DIR/arch/arm/boot/zImage"
    echo "  Device Tree: $KERNEL_BUILD_DIR/arch/arm/boot/dts/ti/omap/am335x-boneblack.dtb"
    echo "  Modules: $YOCTO_BUILD_DIR/image/lib/modules/"
    echo ""
    print_warning "After building manually, run: $0 deploy-direct"
}

# Add alternative function for direct workspace deployment
deploy_kernel_direct() {
    print_status "Deploying kernel directly from build output..."
    
    # Check if kernel build output exists
    KERNEL_IMAGE="$KERNEL_BUILD_DIR/arch/arm/boot/zImage"
    DTB_FILE="$KERNEL_BUILD_DIR/arch/arm/boot/dts/ti/omap/am335x-boneblack.dtb"
    MODULES_DIR="$YOCTO_BUILD_DIR/image/lib/modules"
    
    if [ ! -f "$KERNEL_IMAGE" ]; then
        print_error "Kernel zImage not found at: $KERNEL_IMAGE"
        print_error "Please build the kernel first with devtool"
        echo ""
        echo "Run these commands:"
        echo "cd $YOCTO_BASE_DIR"
        echo "./bitbake_bb_rpi -h bb -d sysv -s"
        echo "devtool modify linux-bb.org    # (if not already done)"
        echo "devtool build linux-bb.org"
        exit 1
    fi
    
    # Deploy kernel
    print_status "Deploying kernel to TFTP..."
    sudo cp "$KERNEL_IMAGE" "$TFTP_DIR/zImage_native_bb"
    sudo chmod 644 "$TFTP_DIR/zImage_native_bb"
    print_success "Kernel deployed as zImage_native_bb"
    
    # Deploy device tree
    if [ -f "$DTB_FILE" ]; then
        print_status "Deploying device tree to TFTP..."
        sudo cp "$DTB_FILE" "$TFTP_DIR/am335x-boneblack.dtb"
        sudo chmod 644 "$TFTP_DIR/am335x-boneblack.dtb"
        print_success "Device tree deployed"
    else
        print_error "Device tree not found at $DTB_FILE"
        exit 1
    fi
    
    # Deploy modules to NFS root
    if [ -d "$MODULES_DIR" ]; then
        print_status "Deploying modules to NFS root..."
        sudo mkdir -p "$NFS_ROOT/lib/modules"
        sudo cp -r "$MODULES_DIR"/* "$NFS_ROOT/lib/modules/"
        print_success "Modules deployed to NFS root"
    else
        print_warning "Kernel modules directory not found at $MODULES_DIR"
    fi
    
    print_success "Direct deployment completed"
    
    # Show deployment summary
    echo ""
    print_status "Deployment Summary:"
    echo "  Kernel: $TFTP_DIR/zImage_native_bb"
    echo "  Device Tree: $TFTP_DIR/am335x-boneblack.dtb" 
    echo "  Modules: $NFS_ROOT/lib/modules/"
    echo ""
}

deploy_kernel_devtool() {
    print_status "Deploying kernel using devtool..."
    
    cd "$YOCTO_BASE_DIR"
    
    # Create temporary deployment directory
    TEMP_DEPLOY_DIR=$(mktemp -d)
    
    # Check if workspace exists
    WORKSPACE_DIR="workspace/sources/linux-bb.org"
    if [ ! -d "$WORKSPACE_DIR" ]; then
        print_error "Workspace directory not found: $WORKSPACE_DIR"
        print_error "Run build first to create workspace"
        rm -rf "$TEMP_DEPLOY_DIR"
        exit 1
    fi
    
    # Copy files from workspace (devtool deploy-target doesn't work well for local TFTP)
    print_status "Copying kernel from workspace..."
    
    # Create deployment structure
    mkdir -p "$TEMP_DEPLOY_DIR/boot"
    mkdir -p "$TEMP_DEPLOY_DIR/lib/modules"
    
    # Copy kernel
    if [ -f "$WORKSPACE_DIR/arch/arm/boot/zImage" ]; then
        cp "$WORKSPACE_DIR/arch/arm/boot/zImage" "$TEMP_DEPLOY_DIR/boot/"
    fi
    
    # Copy device tree
    # Copy device tree
    if [ -f "$WORKSPACE_DIR/arch/arm/boot/dts/ti/omap/am335x-boneblack.dtb" ]; then
        cp "$WORKSPACE_DIR/arch/arm/boot/dts/ti/omap/am335x-boneblack.dtb" "$TEMP_DEPLOY_DIR/boot/"
    fi
    
    # Find and copy modules
    MODULES_SRC=$(find tmp/work -path "*/linux-bb.org/*/image/lib/modules/*" -type d 2>/dev/null | head -1)
    if [ -n "$MODULES_SRC" ] && [ -d "$MODULES_SRC" ]; then
        cp -r "$MODULES_SRC" "$TEMP_DEPLOY_DIR/lib/modules/"
    fi
    
    # Copy files to TFTP directory
    print_status "Copying files to TFTP directory..."
    
    # Copy and rename kernel
    if [ -f "$TEMP_DEPLOY_DIR/boot/zImage" ]; then
        sudo cp "$TEMP_DEPLOY_DIR/boot/zImage" "$TFTP_DIR/zImage_native_bb"
        sudo chmod 644 "$TFTP_DIR/zImage_native_bb"
        print_success "Kernel deployed as zImage_native_bb"
    else
        print_error "zImage not found in deployment"
    fi
    
    # Copy and rename device tree
    if [ -f "$TEMP_DEPLOY_DIR/boot/am335x-boneblack.dtb" ]; then
        sudo cp "$TEMP_DEPLOY_DIR/boot/am335x-boneblack.dtb" "$TFTP_DIR/am335x-boneblack.dtb"
        sudo chmod 644 "$TFTP_DIR/am335x-boneblack.dtb"
        print_success "Device tree deployed"
    else
        print_error "Device tree not found in deployment"
    fi
    
    # Clean up temporary directory
    rm -rf "$TEMP_DEPLOY_DIR"
    
    print_success "Devtool deployment completed"
}

deploy_dtb() {
    print_status "Deploying device tree to TFTP..."
    
    # Find the device tree blob
    DTB_PATH=$(find $YOCTO_BUILD_DIR -name "am335x-boneblack.dtb" -type f | head -1)
    if [ -z "$DTB_PATH" ]; then
        print_error "Device tree blob (am335x-boneblack.dtb) not found in $YOCTO_BUILD_DIR"
        exit 1
    fi
    
    # Copy DTB to TFTP
    cp "$DTB_PATH" "$TFTP_DIR/am335x-boneblack.dtb"
    chmod 644 "$TFTP_DIR/am335x-boneblack.dtb"
    
    print_success "Device tree deployed to $TFTP_DIR/am335x-boneblack.dtb"
}

deploy_modules() {
    print_status "Deploying kernel modules to NFS root..."
    
    # Find modules directory
    MODULES_PATH=$(find $YOCTO_BUILD_DIR -path "*/image/lib/modules/*" -type d | head -1)
    if [ -z "$MODULES_PATH" ]; then
        print_warning "No kernel modules found, skipping module deployment"
        return 0
    fi
    
    # Get kernel version from modules path
    KERNEL_VERSION=$(basename "$MODULES_PATH")
    MODULE_BASE_PATH=$(dirname "$MODULES_PATH")
    
    # Create modules directory in NFS root
    mkdir -p "$NFS_ROOT/lib/modules"
    
    # Remove old modules for this kernel version
    if [ -d "$NFS_ROOT/lib/modules/$KERNEL_VERSION" ]; then
        rm -rf "$NFS_ROOT/lib/modules/$KERNEL_VERSION"
    fi
    
    # Copy new modules
    cp -r "$MODULE_BASE_PATH"/* "$NFS_ROOT/lib/modules/"
    
    print_success "Kernel modules deployed to $NFS_ROOT/lib/modules/"
}

deploy_rootfs() {
    print_status "Deploying root filesystem to NFS..."
    
    # Find rootfs image
    ROOTFS_PATH=$(find /opt/yocto/ycoto-excersise/bb-build-sysv/deploy-ti/images/beaglebone -name "*rootfs.tar.*" | head -1)
    if [ -z "$ROOTFS_PATH" ]; then
        print_warning "Root filesystem archive not found, skipping rootfs deployment"
        return 0
    fi
    
    # Create temporary extraction directory
    TEMP_DIR="/tmp/bb_rootfs_$$"
    mkdir -p "$TEMP_DIR"
    
    # Extract rootfs
    print_status "Extracting rootfs from $ROOTFS_PATH..."
    if [[ "$ROOTFS_PATH" == *.tar.bz2 ]]; then
        tar -xjf "$ROOTFS_PATH" -C "$TEMP_DIR"
    elif [[ "$ROOTFS_PATH" == *.tar.gz ]] || [[ "$ROOTFS_PATH" == *.tar.xz ]]; then
        tar -xf "$ROOTFS_PATH" -C "$TEMP_DIR"
    else
        print_error "Unknown rootfs archive format: $ROOTFS_PATH"
        rm -rf "$TEMP_DIR"
        exit 1
    fi
    
    # Move new rootfs to NFS location
    print_status "Deploying rootfs to $NFS_ROOT..."
    sudo rm -rf "$NFS_ROOT"/*
    sudo mv "$TEMP_DIR"/* "$NFS_ROOT"
    
    print_success "Root filesystem deployed to $NFS_ROOT"
}

show_network_info() {
    print_status "Network boot configuration:"
    echo "  BeagleBone IP: $BB_IP"
    echo "  Server IP: $SERVER_IP" 
    echo "  TFTP Directory: $TFTP_DIR"
    echo "  NFS Root: $NFS_ROOT"
    echo ""
    echo "U-Boot command:"
    echo "  setenv serverip $SERVER_IP"
    echo "  setenv ipaddr $BB_IP"
    echo "  setenv bootcmd 'setenv serverip $SERVER_IP; setenv ipaddr $BB_IP; tftpboot 88000000 am335x-boneblack.dtb; tftpboot 0x82000000 zImage_native_bb; bootz 0x82000000 - 88000000'"
    echo "  saveenv"
    echo "  boot"
}

restart_beaglebone() {
    print_status "Attempting to restart BeagleBone Black..."
    if ping -c 1 "$BB_IP" >/dev/null 2>&1; then
        ssh -o ConnectTimeout=5 root@"$BB_IP" "reboot" 2>/dev/null || true
        print_success "Restart command sent to BeagleBone Black"
    else
        print_warning "BeagleBone Black not reachable at $BB_IP"
        print_status "Please manually restart the BeagleBone Black"
    fi
}

show_usage() {
    echo "Usage: $0 [build|deploy|deploy-direct|all|rootfs|info]"
    echo ""
    echo "Commands:"
    echo "  build         - Build kernel using devtool (modify + build)"
    echo "  deploy        - Deploy kernel, DTB to TFTP using devtool (may fail for local TFTP)"
    echo "  deploy-direct - Deploy kernel, DTB to TFTP directly from workspace (recommended)"
    echo "  all           - Build and deploy using direct method (default)"
    echo "  rootfs        - Deploy complete rootfs to NFS"
    echo "  info          - Show network configuration"
    echo ""
    echo "DevTool Workflow:"
    echo "  1. devtool modify linux-bb.org    (creates workspace)"
    echo "  2. devtool build linux-bb.org     (builds kernel)"
    echo "  3. Use 'deploy-direct' for local TFTP deployment"
    echo ""
}

# Main script logic
case "${1:-all}" in
    "build")
        build_kernel_devtool
        ;;
    
    "deploy")
        check_prerequisites
        deploy_kernel_devtool
        restart_beaglebone
        ;;
        
    "deploy-direct")
        check_prerequisites
        deploy_kernel_direct
        restart_beaglebone
        ;;
    
    "rootfs")
        check_prerequisites
        deploy_rootfs
        ;;
    
    "all")
        print_status "Building and deploying kernel for BeagleBone Black..."
        
        # Build kernel using devtool
        build_kernel_devtool
        
        # Deploy using direct method (more reliable for local TFTP)
        check_prerequisites
        deploy_kernel_direct
        restart_beaglebone
        ;;
    
    "info")
        show_network_info
        ;;
    
    *)
        show_usage
        exit 1
        ;;
esac

if [ "$1" = "deploy" ] || [ "$1" = "all" ]; then
    echo ""
    show_network_info
    echo ""
    print_success "DevTool deployment complete! BeagleBone Black should boot with new kernel."
    echo ""
    print_status "To edit kernel source:"
    echo "  cd /opt/yocto/ycoto-excersise"
    echo "  source poky/5.0.14/oe-init-build-env bb-build-sysv"
    echo "  cd workspace/sources/linux-bb.org"
    echo "  # Edit kernel source files"
    echo "  devtool build linux-bb.org"
    echo "  devtool deploy-target linux-bb.org /srv/tftp"
fi
