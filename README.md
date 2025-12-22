# linux-device-driver-excersise

## Development Environment Setup

### BeagleBone Black Setup
For cross-compiling kernel modules for BeagleBone Black:

```bash
# Navigate to your module directory
cd 3-developing_kernel_modules/code

# Source the BeagleBone environment (SDK 5.0.14, ARMv7)
source ../../prepareEnv/bb-setup/setup_bb.sh

# Build modules for BeagleBone Black
make
```

**Features:**
- ✅ Yocto SDK 5.0.14 with ARMv7 cross-compiler
- ✅ Automatic kernel build directory detection
- ✅ Environment validation and error checking
- ✅ ARM 32-bit architecture support

### Raspberry Pi 4 Setup
For cross-compiling kernel modules for Raspberry Pi 4:

```bash
# Navigate to your module directory
cd 3-developing_kernel_modules/code

# Source the Raspberry Pi environment (SDK 5.0.8, ARM64)
source ../../prepareEnv/rpi-setup/setup_rpi.sh

# Build modules for Raspberry Pi 4
make
```

**Features:**
- ✅ Yocto SDK 5.0.8 with ARM64 cross-compiler
- ✅ Automatic kernel build directory detection
- ✅ Environment validation and error checking
- ✅ ARM 64-bit architecture support

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

## Project Structure

```
linux-device-driver-excersise/
├── prepareEnv/
│   ├── bb-setup/                 # BeagleBone Black setup
│   │   ├── setup_bb.sh          # Main BeagleBone environment setup
│   │   ├── scripts/
│   │   │   └── setup_bb_env.sh  # Core environment configuration
│   │   ├── bb_build_modules.md  # Module building guide
│   │   ├── bb_build_linux.md    # Linux kernel building guide
│   │   └── bb_flash_sd.md       # SD card flashing guide
│   └── rpi-setup/               # Raspberry Pi 4 setup
│       ├── setup_rpi.sh         # Main Raspberry Pi environment setup
│       ├── scripts/
│       │   └── setup_env.sh     # Core environment configuration
│       ├── rpi_build_modules.md # Module building guide
│       └── rpi_flash_sd.md      # SD card flashing guide
├── 3-developing_kernel_modules/
│   └── code/                    # Kernel module source code
└── .vscode/                     # VS Code configuration and tasks
```

## Environment Variables Set by Setup Scripts

### BeagleBone Black (`setup_bb.sh`)
- `ARCH=arm`
- `CROSS_COMPILE=arm-poky-linux-gnueabi-`
- `CC=arm-poky-linux-gnueabi-gcc`
- `KDIR=/opt/yocto/tmp/work/beaglebone-poky-linux-gnueabi/linux-bb.org/6.6.32+git/build`

### Raspberry Pi 4 (`setup_rpi.sh`)
- `ARCH=arm64`
- `CROSS_COMPILE=aarch64-poky-linux-`
- `CC=aarch64-poky-linux-gcc`
- `KDIR=/opt/yocto/tmp/work/raspberrypi4_64-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi-6.6.63+git`

## Notes
- Both setup scripts automatically handle LD_LIBRARY_PATH conflicts
- Scripts must be **sourced** (not executed) to persist environment variables
- Each script validates the development environment before proceeding
- Cross-compiler versions are automatically detected and displayed
