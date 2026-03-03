#!/bin/bash

# BeagleBone Black Yocto SDK Environment Setup
# Adapted from Raspberry Pi setup
# Usage: source ./setup_bb_env.sh  (or . ./setup_bb_env.sh)

# Check if script is being sourced
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "⚠️  This script should be sourced, not executed directly."
    echo "Usage: source ./setup_bb_env.sh"
    echo "   or: . ./setup_bb_env.sh"
    exit 1
fi

# Set up environment variables for BeagleBone Black cross-compilation
echo "Setting up BeagleBone Black Yocto SDK environment..."

# Unset LD_LIBRARY_PATH to avoid SDK conflicts
if [ -n "$LD_LIBRARY_PATH" ]; then
    echo "Unsetting LD_LIBRARY_PATH to avoid SDK conflicts..."
    unset LD_LIBRARY_PATH
fi

# Source the Yocto SDK environment
if [ -f "/opt/yocto/poky/5.0.14/environment-setup-armv7at2hf-neon-poky-linux-gnueabi" ]; then
    echo "Sourcing Yocto SDK environment..."
    . /opt/yocto/poky/5.0.14/environment-setup-armv7at2hf-neon-poky-linux-gnueabi
    echo "✅ Yocto SDK environment loaded"
else
    echo "❌ Yocto SDK not found. Please build the SDK first:"
    echo "   ./bitbake_bb_rpi -h bb -d sysv -t custom-image -c"
    return 1
fi

# Set kernel build directory
export KDIR="/opt/yocto/tmp/work/beaglebone-poky-linux-gnueabi/linux-bb.org/6.6.32+git/linux-bb.org-6.6.32+git"

if [ ! -d "$KDIR" ]; then
    echo "❌ Kernel build directory not found: $KDIR"
    echo "Please build the kernel first: bitbake virtual/kernel"
    return 1
fi

echo "✅ KDIR set to: $KDIR"

# Set deployment directory for BeagleBone Black
export DEPLOY_DIR="/srv/nfs4/bb_busybox"
export DEPLOY_METHOD="copy"

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
echo "🎯 BeagleBone Black development environment ready!"
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
