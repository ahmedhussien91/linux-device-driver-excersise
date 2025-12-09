# VS Code Linux Kernel Development Setup

This workspace is configured for efficient Linux kernel module development with cross-compilation for Raspberry Pi 4.

## 🚀 Quick Start

### Prerequisites
- Yocto SDK installed at `/opt/yocto/poky/5.0.8/`
- Cross-compilation environment set up
- SSH access to your Raspberry Pi 4 target

### Keyboard Shortcuts
- **Ctrl+Shift+B**: Build kernel module (cross-compile for RPi4)
- **Ctrl+Shift+T**: Deploy and test current module (auto-detected from active file)
- **Ctrl+Alt+T**: Deploy and test default module
- **Ctrl+Shift+C**: Clean build artifacts
- **F5**: Start debugging session

## 🎯 **Smart Module Detection**

The deploy script now automatically detects which module you're working on:

1. **From active file**: If you have a `.c` file open, it uses that module name
2. **From current directory**: Looks for `.c` files in the current working directory
3. **From common locations**: Searches standard module directories
4. **Manual override**: You can still specify manually: `./deploy_and_test.sh my_module`

## 🛠️ Available Tasks

### Build Tasks
1. **Build Kernel Module (Cross-compile for RPi4)** - Default build task
2. **Build Local Kernel Module** - Build for local system
3. **Clean Kernel Module** - Remove build artifacts
4. **Build File Handling Module (Cross-compile)**
5. **Build Character Device Module (Cross-compile)**

### Test Tasks
1. **Deploy and Test Current Module** - Auto-detects current module
2. **Deploy and Test Module** - Uses default/fallback module

## 📁 Configuration Files

### `.vscode/c_cpp_properties.json`
- **Linux Kernel Development**: Cross-compilation configuration with proper kernel headers
- **Local Development**: Local system configuration for testing

### `.vscode/tasks.json`
- Pre-configured build tasks for different modules
- Cross-compilation environment setup
- Problem matchers for error detection
- Smart module detection via environment variables

### `.vscode/settings.json`
- Kernel-specific file associations
- Proper indentation (tabs, 8 spaces)
- Code formatting rules following Linux kernel style
- Search exclusions for build artifacts

## 🎯 Workflow

1. **Open** any `.c` kernel module file
2. **Build** with `Ctrl+Shift+B`
3. **Deploy & Test** with `Ctrl+Shift+T` (auto-detects current module!)
4. **Debug** with kernel logs via dmesg

## 📜 Helper Scripts

### `deploy_and_test.sh`
Now with smart auto-detection:
```bash
# Auto-detect from VS Code context
./deploy_and_test.sh

# Or specify manually
./deploy_and_test.sh my_module
```

### `test_module.sh`
Advanced module testing with auto-detection:
```bash
# Auto-detect module
./test_module.sh

# With parameters
./test_module.sh my_module 'param1=value1 param2=value2'

# Just parameters (auto-detect module)
./test_module.sh 'debug=1'
```

## ⚙️ Configuration

### Update Target IP
Edit the TARGET variable in:
- `.vscode/deploy_and_test.sh`
- `.vscode/test_module.sh`

```bash
TARGET="root@192.168.1.100"  # Your RPi4 IP
```

## 🔍 Auto-Detection Logic

The scripts detect modules in this priority order:

1. **Command line argument** (highest priority)
2. **VS Code file context** (`$VSCODE_CWD` environment variable)
3. **Current directory** (if contains Makefile and .c files)
4. **Common module directories**:
   - `3-developing_kernel_modules/code`
   - `1-file_handling/module`
   - `5-devUserSpaceAccess`
5. **Default fallback** (`hello` module)

## 🔍 IntelliSense Features

- **Go to Definition**: F12 or Ctrl+Click
- **Find All References**: Shift+F12
- **Symbol Search**: Ctrl+T
- **Quick Fix**: Ctrl+.
- **Parameter Hints**: Ctrl+Shift+Space

## 🐛 Debugging

### QEMU Debugging (Advanced)
Configure launch.json for QEMU-based kernel debugging with GDB.

### Log-based Debugging
Use `printk()` statements and monitor with:
```bash
ssh root@192.168.1.100 "dmesg -w"
```

## 📚 Extensions Installed

1. **C/C++** (Microsoft) - IntelliSense and debugging
2. **clangd** - Alternative C++ language server
3. **Embedded Linux Kernel Dev** - Kernel-specific support

## 🔧 Troubleshooting

### Build Issues
- Verify Yocto environment: `source /opt/yocto/poky/5.0.8/environment-setup-cortexa72-poky-linux`
- Check kernel source path: `/opt/yocto/ycoto-excersise/rpi-build-sysv/workspace/sources/linux-raspberrypi`

### IntelliSense Issues
- Reload window: `Ctrl+Shift+P` → "Developer: Reload Window"
- Reset IntelliSense: `Ctrl+Shift+P` → "C/C++: Reset IntelliSense Database"

### SSH/Deploy Issues
- Verify SSH key setup
- Check network connectivity
- Confirm target IP address

### Auto-Detection Issues
- Ensure `.c` files are in the expected directories
- Check that Makefile exists in module directories
- Use manual module name as fallback: `./deploy_and_test.sh module_name`

## 📖 Kernel Coding Style

This setup enforces Linux kernel coding style:
- Tabs for indentation (8 spaces wide)
- 80-character line limit (ruler shown)
- K&R brace style
- No trailing whitespace

## 🎉 Happy Kernel Hacking!

Your development environment is now optimized with smart module detection! Just open any kernel module file and press `Ctrl+Shift+T` to automatically build, deploy, and test it.
