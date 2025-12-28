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

After `devtool modify linux-bb.org`, you get a kernel workspace at:

```
/opt/yocto/ycoto-excersise/bb-build-sysv/workspace/sources/linux-bb.org/
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
devtool build linux-bb.org
```

This is faster than bitbake because:

- It does not rebuild rootfs
- It skips do_rootfs, do_image, do_package, etc.
- No dependency scanning of the entire distro

It only rebuilds kernel, modules, dtb.

##### **4. Deploy to BeagleBone Black instantly**

A) **Network Boot Deployment (TFTP + NFS)**

BeagleBone Black is configured for network boot:
- **IP**: 192.168.1.107
- **Server IP**: 192.168.1.11
- **TFTP**: Kernel and DTB from `/srv/tftp`
- **NFS**: Root filesystem from `/srv/nfs4/bb_busybox`

**Boot command:**
```bash
bootcmd=setenv serverip 192.168.1.11; setenv ipaddr 192.168.1.107; tftpboot 88000000 am335x-boneblack.dtb; tftpboot 0x82000000 zImage_native_bb; bootz 0x82000000 - 88000000;
```

**Quick deployment using devtool:**
```bash
# Complete build and deployment
sudo ../../prepareEnv/bb-setup/scripts/deploy_network_boot.sh all

# Or step by step:
sudo ../../prepareEnv/bb-setup/scripts/deploy_network_boot.sh build   # devtool modify + build
sudo ../../prepareEnv/bb-setup/scripts/deploy_network_boot.sh deploy  # devtool deploy-target
```

**Manual devtool workflow:**
```bash
cd /opt/yocto/ycoto-excersise
./bitbake_bb_rpi -h bb -d sysv -s
source poky/5.0.14/oe-init-build-env bb-build-sysv

# Set up kernel workspace (once)
devtool modify linux-bb.org

# Build kernel
devtool build linux-bb.org

# Deploy to TFTP
devtool deploy-target linux-bb.org /srv/tftp

# Copy files to expected names
sudo cp /srv/tftp/boot/zImage /srv/tftp/zImage_native_bb
sudo cp /srv/tftp/boot/am335x-boneblack.dtb /srv/tftp/am335x-boneblack.dtb
```

**Manual deployment:**
```bash
# Deploy kernel to TFTP
sudo cp arch/arm/boot/zImage /srv/tftp/zImage_native_bb

# Deploy device tree to TFTP
sudo cp arch/arm/boot/dts/ti/omap/am335x-boneblack.dtb /srv/tftp/

# Deploy modules to NFS root
sudo cp -r tmp/work/beaglebone-poky-linux-gnueabi/linux-bb.org/*/image/lib/modules/* /srv/nfs4/bb_busybox/lib/modules/

# Restart BeagleBone to load new kernel
ssh root@192.168.1.107 reboot
```

B) **SSH Deployment (if board is reachable)**

```bash
devtool deploy-target linux-yocto root@192.168.1.107
```

##### **5. Alternative: Manual deployment via SD card**

If network boot is not available, you can copy files manually to SD card:

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
