# linux-device-driver-excersise

## Quick Setup

### 1. Install the deploy_kernel alias
```sh
echo 'alias deploy_kernel="/home/ahmed/Documents/Linux_course/linux_device_drivers/Excersise/linux-device-driver-excersise/.vscode/deploy_and_test.sh"' >> ~/.bashrc
source ~/.bashrc
```

### 2. Usage Examples
```sh
# Auto-detect module from current directory
deploy_kernel

# Deploy specific module
deploy_kernel hello
deploy_kernel printk_loglvl

# The script automatically finds your Raspberry Pi at 192.168.1.101
```

### 3. VS Code Integration
- **F6**: Deploy and test current module
- **Ctrl+Shift+B**: Build module
- **Ctrl+Shift+C**: Clean build artifacts
