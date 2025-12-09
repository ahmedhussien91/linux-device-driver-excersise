# ✅ 1. What Yocto produces for Raspberry Pi 4

After building an image:

```
./bitbake_bb_rpi -h rpi -d sysv -t custom-image # build image
```

You will have output files in:

```
tmp/deploy/images/raspberrypi4/
```

The important one for SD card is:

```
custom-image-raspberrypi4-64.rootfs.rpi-sdimg
```

Example:
 `core-image-minimal-raspberrypi4-20250101.wic`

This is an SD-card–ready image that already contains:

- FAT32 **boot** partition (kernel + bootloader + dtbs + config.txt)
- ext4 **rootfs**
- possibly a miscellaneous partition (depends on your build)

------

# ✅ 2. Flash the RPi SD card

### On **Linux**

use raw `dd`:

```
sudo dd if=/opt/yocto/tmp/deploy/images/raspberrypi4-64/custom-image-raspberrypi4-64.rootfs.rpi-sdimg of=/dev/sde bs=4M status=progress conv=fsync
```

> 2239758336 bytes (2.2 GB, 2.1 GiB) copied, 8 s, 280 MB/s2303721472 bytes (2.3 GB, 2.1 GiB) copied, 8.21272 s, 281 MB/s
>
> 
>
> 549+1 records in
> 549+1 records out
> 2303721472 bytes (2.3 GB, 2.1 GiB) copied, 148.866 s, 15.5 MB/s

To identify the SD device:

```
lsblk
```

⚠️ Make sure `/dev/sdX` is the **device**, not a partition (`/dev/sdX1`).