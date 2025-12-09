#!/usr/bin/env bash
set -e

# Configuration - Update these values for your target
TARGET="root@192.168.1.101"  # Update with your RPi4 IP address

# Remember original working directory
ORIGINAL_PWD="$(pwd)"

# Find workspace root (look for .vscode directory)
WORKSPACE_ROOT=""
CURRENT_DIR="$ORIGINAL_PWD"
while [ "$CURRENT_DIR" != "/" ]; do
    if [ -d "$CURRENT_DIR/.vscode" ] && [ -f "$CURRENT_DIR/.vscode/tasks.json" ]; then
        WORKSPACE_ROOT="$CURRENT_DIR"
        break
    fi
    CURRENT_DIR="$(dirname "$CURRENT_DIR")"
done

# If not found, try script directory
if [ -z "$WORKSPACE_ROOT" ]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    WORKSPACE_ROOT="$(dirname "$SCRIPT_DIR")"
fi

echo "Using workspace: $WORKSPACE_ROOT"

# Auto-detect current working module
# Priority: 1. Command line argument, 2. Current file context, 3. Default
if [ -n "$1" ]; then
    # Remove .ko extension if provided
    MODULE_NAME="${1%.ko}"
    echo "Using module from command line: $MODULE_NAME"
    
    # Now find where this module is located
    if [ -f "$ORIGINAL_PWD/${MODULE_NAME}.ko" ]; then
        MODULE_DIR="$ORIGINAL_PWD"
        echo "Found ${MODULE_NAME}.ko in current directory: $MODULE_DIR"
    else
        # Search in common module directories
        for dir in "3-developing_kernel_modules/code" "1-file_handling/module" "5-devUserSpaceAccess"; do
            if [ -f "$WORKSPACE_ROOT/$dir/${MODULE_NAME}.ko" ]; then
                MODULE_DIR="$WORKSPACE_ROOT/$dir"
                echo "Found ${MODULE_NAME}.ko in: $dir"
                break
            fi
        done
        
        # If still not found, default to current directory or common location
        if [ -z "$MODULE_DIR" ]; then
            if [ -f "$ORIGINAL_PWD/Makefile" ]; then
                MODULE_DIR="$ORIGINAL_PWD"
                echo "Module not found, using current directory: $MODULE_DIR"
            else
                MODULE_DIR="$WORKSPACE_ROOT/3-developing_kernel_modules/code"
                echo "Module not found, using default directory: $MODULE_DIR"
            fi
        fi
    fi
elif [ -n "$VSCODE_CWD" ] && [ -f "$VSCODE_CWD/Makefile" ]; then
    # If called from VS Code task, use the current working directory
    MODULE_DIR="$VSCODE_CWD"
    MODULE_NAME=$(basename "$(find "$MODULE_DIR" -name "*.c" -not -name "*.mod.c" | head -1)" .c 2>/dev/null || echo "hello")
    echo "Auto-detected from VS Code context: $MODULE_NAME in $MODULE_DIR"
else
    # Check if we're currently in a module directory
    if [ -f "$ORIGINAL_PWD/Makefile" ] && [ -n "$(find "$ORIGINAL_PWD" -maxdepth 1 -name "*.c" -not -name "*.mod.c" 2>/dev/null)" ]; then
        MODULE_DIR="$ORIGINAL_PWD"
        MODULE_NAME=$(basename "$(find "$ORIGINAL_PWD" -maxdepth 1 -name "*.c" -not -name "*.mod.c" | head -1)" .c 2>/dev/null || echo "hello")
        echo "Auto-detected from original directory: $MODULE_NAME in $MODULE_DIR"
    else
        # Check common module directories relative to workspace root
        for dir in "3-developing_kernel_modules/code" "1-file_handling/module" "5-devUserSpaceAccess"; do
            if [ -d "$WORKSPACE_ROOT/$dir" ] && [ -n "$(find "$WORKSPACE_ROOT/$dir" -name "*.c" -not -name "*.mod.c" 2>/dev/null)" ]; then
                MODULE_DIR="$WORKSPACE_ROOT/$dir"
                MODULE_NAME=$(basename "$(find "$MODULE_DIR" -name "*.c" -not -name "*.mod.c" | head -1)" .c 2>/dev/null || echo "hello")
                echo "Auto-detected from $dir: $MODULE_NAME"
                break
            fi
        done
        
        # Final fallback
        if [ -z "$MODULE_NAME" ]; then
            MODULE_DIR="$WORKSPACE_ROOT/3-developing_kernel_modules/code"
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
    echo -e "${RED}❌ Error: ${MODULE_NAME}.ko not found in ${MODULE_DIR}${NC}"
    echo -e "${YELLOW}💡 Available .ko files:${NC}"
    find "$WORKSPACE_ROOT" -name "*.ko" -type f 2>/dev/null | while read -r ko_file; do
        echo "  - $(basename "$ko_file" .ko) ($(dirname "$ko_file"))"
    done
    echo -e "${YELLOW}💡 Run the build task first (Ctrl+Shift+B) or specify one of the available modules${NC}"
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
