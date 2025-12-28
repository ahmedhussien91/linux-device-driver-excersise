#!/bin/bash

# BeagleBone Black Kernel Module Deployment Script
# Deploys built kernel modules to NFS root filesystem
# Usage: ./deploy_modules.sh [module_path]

set -e

# Configuration
NFS_ROOT="/srv/nfs4/bb_busybox"
BB_IP="192.168.1.107"

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

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_nfs_root() {
    if [ ! -d "$NFS_ROOT" ]; then
        print_error "NFS root directory $NFS_ROOT not found"
        exit 1
    fi
    
    if [ ! -w "$NFS_ROOT" ]; then
        print_error "No write permission to $NFS_ROOT. Run with sudo."
        exit 1
    fi
}

deploy_current_modules() {
    local module_dir=${1:-$(pwd)}
    
    print_status "Deploying kernel modules from $module_dir to NFS root..."
    
    # Find .ko files in current directory
    local ko_files=($(find "$module_dir" -maxdepth 1 -name "*.ko" -type f))
    
    if [ ${#ko_files[@]} -eq 0 ]; then
        print_error "No kernel modules (.ko files) found in $module_dir"
        print_status "Build modules first with: make"
        exit 1
    fi
    
    # Create modules directory in NFS if it doesn't exist
    local kernel_version=$(uname -r 2>/dev/null || echo "6.6.32-yocto-standard")
    local nfs_modules_dir="$NFS_ROOT/lib/modules/$kernel_version/extra"
    
    mkdir -p "$nfs_modules_dir"
    
    # Copy each module
    for ko_file in "${ko_files[@]}"; do
        local module_name=$(basename "$ko_file")
        print_status "Copying $module_name..."
        cp "$ko_file" "$nfs_modules_dir/"
        chmod 644 "$nfs_modules_dir/$module_name"
    done
    
    # Update module dependencies
    if command -v depmod >/dev/null 2>&1; then
        print_status "Updating module dependencies..."
        depmod -a -b "$NFS_ROOT" "$kernel_version" 2>/dev/null || true
    fi
    
    print_success "Deployed ${#ko_files[@]} modules to $nfs_modules_dir"
    
    # List deployed modules
    print_status "Deployed modules:"
    for ko_file in "${ko_files[@]}"; do
        echo "  - $(basename "$ko_file")"
    done
}

test_module_load() {
    local module_name=${1%%.ko}  # Remove .ko extension if present
    
    if ping -c 1 "$BB_IP" >/dev/null 2>&1; then
        print_status "Testing module $module_name on BeagleBone Black..."
        
        ssh -o ConnectTimeout=5 root@"$BB_IP" "
            echo 'Loading module $module_name...'
            modprobe $module_name && echo 'Module loaded successfully' || echo 'Module load failed'
            lsmod | grep $module_name || echo 'Module not found in lsmod'
            echo 'Recent dmesg output:'
            dmesg | tail -5
        " 2>/dev/null || print_error "Could not connect to BeagleBone Black at $BB_IP"
    else
        print_error "BeagleBone Black not reachable at $BB_IP"
        print_status "Manually load modules on BeagleBone with:"
        echo "  insmod /lib/modules/\$(uname -r)/extra/MODULE_NAME.ko"
        echo "  or"
        echo "  modprobe MODULE_NAME"
    fi
}

show_usage() {
    echo "Usage: $0 [options] [module_directory]"
    echo ""
    echo "Options:"
    echo "  -t, --test MODULE    Test load specific module on BeagleBone"
    echo "  -h, --help          Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                           # Deploy modules from current directory"
    echo "  $0 /path/to/modules         # Deploy modules from specific directory"
    echo "  $0 --test hello             # Test loading hello.ko module"
    echo ""
    echo "NFS Root: $NFS_ROOT"
    echo "BeagleBone IP: $BB_IP"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--test)
            test_module_load "$2"
            exit 0
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            MODULE_DIR="$1"
            shift
            ;;
    esac
done

# Main execution
check_nfs_root
deploy_current_modules "${MODULE_DIR:-$(pwd)}"

echo ""
print_status "To test modules on BeagleBone Black:"
print_status "  ssh root@$BB_IP"
print_status "  modprobe MODULE_NAME"
print_status "  dmesg | tail"
