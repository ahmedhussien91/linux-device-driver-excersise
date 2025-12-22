##### **1. Use `devtool modify` — your controlled playground**

`devtool modify` is EXTREMELY fast compared to editing Yocto recipes manually.

It creates a working copy of the kernel sources **outside tmp/**, stable, not deleted by bitbake.

**Run this once:**

```bash
devtool modify linux-yocto
```

This gives you:

📌 **Workspace source directory:**
 `workspace/sources/linux-yocto/`

📌 **Your bbappend:**
 `workspace/appends/linux-yocto.bbappend`

##### **2. Edit kernel source directly like a normal Git repo**

You now treat this directory like your own kernel repo:

```
workspace/sources/linux-yocto/
    |-- drivers/
    |-- arch/
    |-- include/
    |-- scripts/
    |-- Documentation/
```

Modify anything:

- C code
- DTS/DTSI files
- Kconfig
- Makefiles

This is *your* playground.

##### **3. Rebuild JUST the kernel (super fast)**

```bash
devtool build linux-yocto
```

This is faster than bitbake because:

- It does not rebuild rootfs
- It skips do_rootfs, do_image, do_package, etc.
- No dependency scanning of the entire distro

It only rebuilds kernel, modules, dtb.

##### **4. Deploy to BeagleBone Black instantly**

A) If the board is reachable via SSH:

```bash
devtool deploy-target linux-yocto root@192.168.1.101
```

This copies:

- `zImage`
- DTBs
- Overlays
- Kernel modules

directly into the correct directories on the running filesystem.

After that, just reboot:

```bash
ssh root@192.168.1.101 reboot
```

##### **5. Alternative: Manual deployment via SD card**

If SSH is not available, you can copy files manually:

1. Build the kernel:
   ```bash
   devtool build linux-yocto
   ```

2. Mount the SD card boot partition (usually FAT16/FAT32):
   ```bash
   sudo mount /dev/sdX1 /mnt/boot
   ```

3. Copy kernel and device tree:
   ```bash
   sudo cp workspace/sources/linux-yocto/arch/arm/boot/zImage /mnt/boot/
   sudo cp workspace/sources/linux-yocto/arch/arm/boot/dts/ti/omap/am335x-boneblack.dtb /mnt/boot/
   ```

4. Mount rootfs partition and copy modules:
   ```bash
   sudo mount /dev/sdX2 /mnt/rootfs
   sudo cp -r tmp/work/beaglebone_yocto-poky-linux-gnueabi/linux-yocto/*/image/lib/modules/* /mnt/rootfs/lib/modules/
   ```

5. Unmount and boot:
   ```bash
   sudo umount /mnt/boot /mnt/rootfs
   ```

#### **6. Debugging kernel changes**

View kernel logs on BeagleBone:
```bash
dmesg | tail -n 50
```

Check loaded modules:
```bash
lsmod
```

Monitor kernel messages in real-time:
```bash
dmesg -w
```
