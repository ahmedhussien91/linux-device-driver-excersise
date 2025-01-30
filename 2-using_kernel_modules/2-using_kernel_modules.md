# Linux Kernel Module

- Linux kernel modules are kernel code that can be loaded and unloaded into the kernel upon demand.
- They extend the functionality of the kernel without the need to reboot the system.
- This modularity allows for a more flexible and efficient system, as only the necessary components are loaded. For example, a network driver module can be loaded when a new network card is detected, or a file-system module can be loaded to support a new type of file-system.
- They can provide support for new hardware, file systems, or system calls, among other things. 
- Modules are typically used for drivers, which allow the kernel to interact with hardware devices.

## Loading and Unloading Modules

Modules can be loaded into the kernel using the `insmod` or `modprobe` commands. The `modprobe` command is more commonly used because it handles dependencies automatically. For example:
```bash
sudo modprobe <module_name>
```

To unload a module, the `rmmod` command is used:
```bash
sudo rmmod <module_name>
```

The `lsmod` command can be used to list all currently loaded modules:
```bash
lsmod
```

By using these commands, administrators can dynamically manage the kernel's capabilities, loading and unloading modules as needed to support various hardware and software requirements.

# Exercise: Using Kernel Modules

In this exercise, you will learn how to use kernel modules by demonstrating the usage of VLAN support as a module.

## Steps

1. **Build Kernel with VLAN Support as a Module**
    - Ensure you have the kernel source code and necessary build tools installed.
    - Configure the kernel to build VLAN support as a module:
      ```bash
      make menuconfig
      ```
      - Navigate to `Device Drivers` -> `Network device support` -> `Networking options` -> `802.1Q VLAN Support`
      - Select `M` to build it as a module.
    - Build the kernel and modules:
      ```bash
      make -j$(nproc)
      make modules
      ```

2. **Install Kernel Modules on the Target**
    - Copy the newly built kernel and modules to the target system.
    - Install the kernel and modules on the target network file system (nfs_path):
      ```bash
      make INSTALL_MOD_PATH=<nfs_path> modules_install
      ```

3. **Try to Create VLAN Without Loading the Module**
    - On Target try to create a VLAN interface without loading the VLAN module:
      ```bash
      sudo ip link add link eth0 name eth0.100 type vlan id 100
      ```
    - You should receive an error because the VLAN module is not loaded.

4. **Load the VLAN Module**
    - Load the VLAN module manually:
      ```bash
      sudo modprobe 8021q
      ```

5. **Try to Create the VLAN Again**
   
    - On Target try  to create the VLAN interface again:
      ```bash
      sudo ip link add link eth0 name eth0.100 type vlan id 100
      ```
    - This time, the command should succeed, indicating that the VLAN module is loaded and functioning.

## Conclusion

By following these steps, you have demonstrated how to build, install, and use kernel modules, specifically for VLAN support. This exercise highlights the importance of kernel modules and how they can be dynamically loaded and unloaded as needed.

