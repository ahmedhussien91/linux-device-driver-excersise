#!/usr/bin/env bash
set -e

# Configuration - Update these values for your target
TARGET="root@192.168.1.100"  # Update with your RPi4 IP address

# Auto-detect current working module
# Priority: 1. Command line argument, 2. Current file context, 3. Default
if [ -n "$1" ]; then
    MODULE_NAME="$1"
    echo "Using module from command line: $MODULE_NAME"
elif [ -n "$VSCODE_CWD" ] && [ -f "$VSCODE_CWD/Makefile" ]; then
    # If called from VS Code task, use the current working directory
    MODULE_DIR="$VSCODE_CWD"
    MODULE_NAME=$(basename "$(find "$MODULE_DIR" -name "*.c" -not -name "*.mod.c" | head -1)" .c 2>/dev/null || echo "hello")
    echo "Auto-detected from VS Code context: $MODULE_NAME in $MODULE_DIR"
else
    # Try to detect from current directory or fall back to common locations
    if [ -f "Makefile" ]; then
        MODULE_DIR="$(pwd)"
        MODULE_NAME=$(basename "$(find . -maxdepth 1 -name "*.c" -not -name "*.mod.c" | head -1)" .c 2>/dev/null || echo "hello")
        echo "Auto-detected from current directory: $MODULE_NAME"
    else
        # Check common module directories
        for dir in "3-developing_kernel_modules/code" "1-file_handling/module" "5-devUserSpaceAccess"; do
            if [ -d "$dir" ] && [ -n "$(find "$dir" -name "*.c" -not -name "*.mod.c" 2>/dev/null)" ]; then
                MODULE_DIR="$dir"
                MODULE_NAME=$(basename "$(find "$dir" -name "*.c" -not -name "*.mod.c" | head -1)" .c 2>/dev/null || echo "hello")
                echo "Auto-detected from $dir: $MODULE_NAME"
                break
            fi
        done
        
        # Final fallback
        if [ -z "$MODULE_NAME" ]; then
            MODULE_DIR="3-developing_kernel_modules/code"
            MODULE_NAME="hello"
            echo "Using default: $MODULE_NAME in $MODULE_DIR"
        fi
    fi
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🚀 Deploying and testing kernel module...${NC}"

# Check if .ko file exists
if [ ! -f "${MODULE_DIR}/${MODULE_NAME}.ko" ]; then
    echo -e "${RED}❌ Error: ${MODULE_NAME}.ko not found in ${MODULE_DIR}/${NC}"
    echo -e "${YELLOW}💡 Run the build task first (Ctrl+Shift+B)${NC}"
    exit 1
fi

echo -e "${YELLOW}📦 Copying ${MODULE_NAME}.ko to target...${NC}"
scp "${MODULE_DIR}/${MODULE_NAME}.ko" "$TARGET:/tmp/" || {
    echo -e "${RED}❌ Failed to copy module to target. Check SSH connection and IP address.${NC}"
    exit 1
}

echo -e "${YELLOW}🔧 Loading module on target...${NC}"
ssh "$TARGET" "
    # Remove module if already loaded
    rmmod ${MODULE_NAME} 2>/dev/null || echo 'Module not currently loaded'
    
    # Load the new module
    insmod /tmp/${MODULE_NAME}.ko && echo -e '${GREEN}✅ Module loaded successfully${NC}' || echo -e '${RED}❌ Failed to load module${NC}'
    
    # Show recent kernel messages
    echo -e '${BLUE}📋 Recent kernel messages:${NC}'
    dmesg | tail -n 20
    
    # Show loaded modules
    echo -e '${BLUE}📋 Loaded modules containing \"${MODULE_NAME}\":${NC}'
    lsmod | grep ${MODULE_NAME} || echo 'Module not found in lsmod'
" || {
    echo -e "${RED}❌ Failed to execute commands on target${NC}"
    exit 1
}

echo -e "${GREEN}🎉 Deploy and test completed!${NC}"
