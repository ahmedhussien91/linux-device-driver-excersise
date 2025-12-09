#!/usr/bin/env bash

# Module Testing Helper Script
# Usage: ./test_module.sh <module_name> [parameters]

set -e

TARGET="root@192.168.1.100"  # Update with your RPi4 IP
MODULE_NAME="${1:-hello}"
PARAMS="${2:-}"

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

# Check if module file exists
if [ ! -f "3-developing_kernel_modules/code/${MODULE_NAME}.ko" ]; then
    echo -e "${RED}❌ Module file ${MODULE_NAME}.ko not found${NC}"
    exit 1
fi

echo -e "${YELLOW}📦 Copying ${MODULE_NAME}.ko to target...${NC}"
scp "3-developing_kernel_modules/code/${MODULE_NAME}.ko" "$TARGET:/tmp/"

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
