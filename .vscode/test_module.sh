#!/usr/bin/env bash

# Module Testing Helper Script
# Usage: ./test_module.sh [module_name] [parameters]
# If no module name provided, attempts to auto-detect from current context

set -e

TARGET="root@192.168.1.101"  # Update with your RPi4 IP

# Auto-detect module name if not provided
if [ -n "$1" ] && [ "$1" != "-h" ] && [ "$1" != "--help" ]; then
    MODULE_NAME="$1"
    PARAMS="${2:-}"
else
    # Auto-detection logic
    if [ -n "$VSCODE_CWD" ] && [ -f "$VSCODE_CWD/Makefile" ]; then
        MODULE_NAME=$(basename "$(find "$VSCODE_CWD" -name "*.c" -not -name "*.mod.c" | head -1)" .c 2>/dev/null || echo "")
        MODULE_DIR="$VSCODE_CWD"
    elif [ -f "Makefile" ]; then
        MODULE_NAME=$(basename "$(find . -maxdepth 1 -name "*.c" -not -name "*.mod.c" | head -1)" .c 2>/dev/null || echo "")
        MODULE_DIR="$(pwd)"
    else
        # Check common directories
        for dir in "3-developing_kernel_modules/code" "1-file_handling/module" "5-devUserSpaceAccess"; do
            if [ -d "$dir" ] && [ -n "$(find "$dir" -name "*.c" -not -name "*.mod.c" 2>/dev/null)" ]; then
                MODULE_NAME=$(basename "$(find "$dir" -name "*.c" -not -name "*.mod.c" | head -1)" .c)
                MODULE_DIR="$dir"
                break
            fi
        done
    fi
    
    # Default fallback
    if [ -z "$MODULE_NAME" ]; then
        MODULE_NAME="hello"
        MODULE_DIR="3-developing_kernel_modules/code"
    fi
    
    PARAMS="${1:-}"  # If first arg wasn't module name, treat it as parameters
fi

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ -z "$MODULE_NAME" ]; then
    echo -e "${RED}Usage: $0 <module_name> [parameters]${NC}"
    echo -e "${YELLOW}Example: $0 hello${NC}"
    echo -e "${YELLOW}Example: $0 modparam 'myint=42 mystring=\"test\"'${NC}"
    exit 1
fi

echo -e "${BLUE}🧪 Testing module: ${MODULE_NAME}${NC}"

# Function to run remote command
run_remote() {
    ssh "$TARGET" "$1"
}

# Determine module directory if not set
if [ -z "$MODULE_DIR" ]; then
    # Search for the module in common directories
    for dir in "3-developing_kernel_modules/code" "1-file_handling/module" "5-devUserSpaceAccess" "."; do
        if [ -f "${dir}/${MODULE_NAME}.ko" ]; then
            MODULE_DIR="$dir"
            break
        fi
    done
fi

# Check if module file exists
if [ ! -f "${MODULE_DIR}/${MODULE_NAME}.ko" ]; then
    echo -e "${RED}❌ Module file ${MODULE_NAME}.ko not found in ${MODULE_DIR}${NC}"
    echo -e "${YELLOW}💡 Available .ko files:${NC}"
    find . -name "*.ko" -type f 2>/dev/null || echo "No .ko files found"
    exit 1
fi

echo -e "${YELLOW}📦 Copying ${MODULE_NAME}.ko to target...${NC}"
scp "${MODULE_DIR}/${MODULE_NAME}.ko" "$TARGET:/tmp/"

echo -e "${YELLOW}🧹 Cleaning up previous module instances...${NC}"
run_remote "rmmod ${MODULE_NAME} 2>/dev/null || true"

echo -e "${YELLOW}📥 Loading module with parameters: ${PARAMS}${NC}"
if [ -n "$PARAMS" ]; then
    run_remote "insmod /tmp/${MODULE_NAME}.ko ${PARAMS}"
else
    run_remote "insmod /tmp/${MODULE_NAME}.ko"
fi

echo -e "${BLUE}📋 Module information:${NC}"
run_remote "lsmod | grep ${MODULE_NAME}"

echo -e "${BLUE}📋 Module parameters:${NC}"
run_remote "find /sys/module/${MODULE_NAME}/parameters/ -type f -exec sh -c 'echo \"\$1: \$(cat \$1)\"' _ {} \\; 2>/dev/null || echo 'No parameters found'"

echo -e "${BLUE}📋 Recent kernel messages:${NC}"
run_remote "dmesg | tail -n 15"

echo -e "${BLUE}🔧 Module info:${NC}"
run_remote "modinfo ${MODULE_NAME} 2>/dev/null || modinfo /tmp/${MODULE_NAME}.ko"

echo -e "${GREEN}✅ Module testing completed!${NC}"
echo -e "${YELLOW}💡 To remove: ssh $TARGET 'rmmod ${MODULE_NAME}'${NC}"
