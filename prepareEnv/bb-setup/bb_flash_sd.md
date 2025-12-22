# ✅ 1. What Yocto produces for BeagleBone Black

After building an image:

```bash
./bitbake_bb_rpi -h bb -d sysv -t custom-image # build image
```

You will have output files in:

```
tmp/deploy/images/beaglebone-yocto/
```

The important one for SD card is:

```
custom-image-beaglebone-yocto.wic
```

Example:
 `core-image-minimal-beaglebone-yocto-20250101.wic`

This is an SD-card–ready image that already contains:

- FAT16 **boot** partition (MLO, u-boot.img, zImage, device tree, uEnv.txt)
- ext4 **rootfs**

------

# ✅ 2. Flash the BeagleBone Black SD card

### On **Linux**

Use raw `dd`:

```bash
sudo dd if=/opt/yocto/tmp/deploy/images/beaglebone-yocto/custom-image-beaglebone-yocto.wic of=/dev/sde bs=4M status=progress conv=fsync
```

> 2239758336 bytes (2.2 GB, 2.1 GiB) copied, 8 s, 280 MB/s2303721472 bytes (2.3 GB, 2.1 GiB) copied, 8.21272 s, 281 MB/s
>
> 549+1 records in
> 549+1 records out
> 2303721472 bytes (2.3 GB, 2.1 GiB) copied, 148.866 s, 15.5 MB/s

To identify the SD device:

```bash
lsblk
```

⚠️ Make sure `/dev/sdX` is the **device**, not a partition (`/dev/sdX1`).

### Alternative: Using bmaptool (faster and safer)

```bash
sudo bmaptool copy --nobmap /opt/yocto/tmp/deploy/images/beaglebone-yocto/custom-image-beaglebone-yocto.wic /dev/sde
```

# ✅ 3. BeagleBone Black Boot Configuration

## Boot Sequence

BeagleBone Black boot sequence:
1. **ROM** → **MLO** → **u-boot.img** → **zImage** + **device tree**

## uEnv.txt Configuration

The boot partition should contain a `uEnv.txt` file for boot configuration:

```bash
# Default Yocto uEnv.txt for BeagleBone Black
console=ttyS0,115200n8
fdtfile=am335x-boneblack.dtb
mmcroot=/dev/mmcblk0p2 rw
mmcrootfstype=ext4 rootwait
optargs=quiet drm.debug=7 capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN

# Load addresses
loadaddr=0x82000000
fdtaddr=0x88000000

# Boot command
uenvcmd=fatload mmc 0:1 ${loadaddr} zImage; fatload mmc 0:1 ${fdtaddr} ${fdtfile}; bootz ${loadaddr} - ${fdtaddr}
```

# Debugging

```bash
# Check filesystem
sudo losetup -Pf custom-image-beaglebone-yocto.wic

# Flash wic image
umount /media/ahmed/boot
umount /media/ahmed/rootfs
sudo umount /media/ahmed/*

# Flashing method one (with bmaptool)
sudo bmaptool copy --nobmap /opt/yocto/tmp/deploy/images/beaglebone-yocto/custom-image-beaglebone-yocto.wic /dev/sde

# Flashing method two (with dd)
sudo dd if=/opt/yocto/tmp/deploy/images/beaglebone-yocto/custom-image-beaglebone-yocto.wic of=/dev/sde bs=4M status=progress conv=fsync

sync

# Manual boot partition setup if needed
cd /opt/yocto/tmp/deploy/images/beaglebone-yocto/bootloader
sudo cp MLO u-boot.img /media/ahmed/boot/
cd ..
sudo cp zImage am335x-boneblack.dtb /media/ahmed/boot/
sudo cp uEnv.txt /media/ahmed/boot/
```

## Boot Configuration Details

### uEnv.txt for Development

For kernel development, you might want more verbose output:

```bash
# Development uEnv.txt
console=ttyS0,115200n8
fdtfile=am335x-boneblack.dtb
mmcroot=/dev/mmcblk0p2 rw
mmcrootfstype=ext4 rootwait
optargs=earlyprintk loglevel=8

# Load addresses
loadaddr=0x82000000
fdtaddr=0x88000000

# Boot command
uenvcmd=fatload mmc 0:1 ${loadaddr} zImage; fatload mmc 0:1 ${fdtaddr} ${fdtfile}; bootz ${loadaddr} - ${fdtaddr}
```

### Common BeagleBone Black Boot Issues

1. **No MLO found**: Ensure MLO is in the root of the boot partition
2. **Wrong device tree**: Make sure `am335x-boneblack.dtb` matches your board revision
3. **HDMI issues**: Add `capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN` to optargs
4. **Serial console**: Ensure console=ttyS0,115200n8 is set correctly
