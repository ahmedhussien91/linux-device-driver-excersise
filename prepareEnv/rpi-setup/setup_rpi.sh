#!/bin/bash

# Raspberry Pi 4 Environment Setup Wrapper
# This script can be sourced from any directory

# Get the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source the main setup script
source "${SCRIPT_DIR}/scripts/setup_env.sh"
