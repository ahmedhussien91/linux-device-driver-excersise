#!/bin/bash

# Raspberry Pi 4 Yocto SDK Environment Setup
# Based on BeagleBone setup
# Usage: source ./setup_env.sh  (or . ./setup_env.sh)

# Check if script is being sourced
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "⚠️  This script should be sourced, not executed directly."
    echo "Usage: source ./setup_env.sh"
    echo "   or: . ./setup_env.sh"
    exit 1
fi

# Set up environment variables for Raspberry Pi 4 cross-compilation
echo "Setting up Raspberry Pi 4 Yocto SDK environment..."

# Unset LD_LIBRARY_PATH to avoid SDK conflicts
if [ -n "$LD_LIBRARY_PATH" ]; then
    echo "Unsetting LD_LIBRARY_PATH to avoid SDK conflicts..."
    unset LD_LIBRARY_PATH
fi

# Source the Yocto SDK environment
if [ -f "/opt/yocto/poky/5.0.8/environment-setup-cortexa72-poky-linux" ]; then
    echo "Sourcing Yocto SDK environment..."
    . /opt/yocto/poky/5.0.8/environment-setup-cortexa72-poky-linux
    echo "✅ Yocto SDK environment loaded"
else
    echo "❌ Yocto SDK not found. Please build the SDK first:"
    echo "   bitbake -c populate_sdk <image-name>"
    return 1
fi

# Set kernel build directory - try to find the correct one
POSSIBLE_KDIRS=(
    "/opt/yocto/tmp/work/raspberrypi4_64-poky-linux/linux-raspberrypi/6.6.63+git/build"
    "/opt/yocto/tmp/work/raspberrypi4_64-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi-6.6.63+git"
)

for kdir in "${POSSIBLE_KDIRS[@]}"; do
    if [ -d "$kdir" ]; then
        export KDIR="$kdir"
        break
    fi
done

if [ -z "$KDIR" ]; then
    echo "❌ Kernel build directory not found. Tried:"
    for kdir in "${POSSIBLE_KDIRS[@]}"; do
        echo "   $kdir"
    done
    echo "Please build the kernel first: bitbake virtual/kernel"
    return 1
fi

echo "✅ KDIR set to: $KDIR"

# Set deployment directory for Raspberry Pi 4
export DEPLOY_DIR="root@192.168.1.101:/tmp/"
export DEPLOY_METHOD="scp"

if [ ! -d "$DEPLOY_DIR" ]; then
    echo "⚠️  Deployment directory not found: $DEPLOY_DIR"
    echo "Please ensure NFS root is properly mounted"
fi

echo "✅ DEPLOY_DIR set to: $DEPLOY_DIR"

# Verify cross-compilation setup
echo "Verifying cross-compilation setup..."
echo "ARCH: $ARCH"
echo "CROSS_COMPILE: $CROSS_COMPILE"
echo "CC: $CC"

if [ -n "$CC" ]; then
    echo "✅ Cross-compiler version:"
    $CC --version | head -1
else
    echo "❌ Cross-compiler not properly set up"
    return 1
fi

echo ""
echo "🎯 Raspberry Pi 4 development environment ready!"
echo "You can now build kernel modules with: make"
echo ""
echo "Example module Makefile should include:"
echo "KDIR ?= $KDIR"
echo ""
echo "Environment variables set:"
echo "  ARCH=$ARCH"
echo "  CROSS_COMPILE=$CROSS_COMPILE"
echo "  KDIR=$KDIR"
echo "  DEPLOY_DIR=$DEPLOY_DIR"
echo "  DEPLOY_METHOD=$DEPLOY_METHOD"
