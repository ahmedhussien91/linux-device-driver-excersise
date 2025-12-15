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





# Debugging 

```sh
#check filesystem
sudo losetup -Pf custom-image-raspberrypi4-64.rootfs.wic

#flash wic image
umount /media/ahmed/boot
umount /media/ahmed/root
sudo umount /media/ahmed/*
# flashing one
sudo bmaptool copy --nobmap /opt/yocto/tmp/deploy/images/raspberrypi4-64/custom-image-raspberrypi4-64.rootfs.wic /dev/sde
# flashing two
sudo dd if=/opt/yocto/tmp/deploy/images/raspberrypi4-64/custom-image-raspberrypi4-64.rootfs.wic of=/dev/sde bs=4M status=progress conv=fsync
sync
cd /opt/yocto/tmp/deploy/images/raspberrypi4-64/bootfiles
sudo cp * /media/ahmed/boot1
sudo mkdir /media/ahmed/boot1/overlays
sudo cp ../*.dtb /media/ahmed/boot1
sudo cp ../*.dtbo /media/ahmed/boot1/overlays
sudo vim /media/ahmed/boot1/config.txt
```

## boot config.txt

add

```sh
# Force HDMI even if no display is detected
hdmi_force_hotplug=1
hdmi_group=1      # 1 = CEA (TV), 2 = DMT (monitor)
hdmi_mode=16      # 1080p60
hdmi_drive=2      # HDMI with audio

# Avoid 4K if unsupported
hdmi_enable_4kp60=0

# Optional: disable camera if suspected
start_x=0
gpu_mem=64

# Enable console on HDMI
enable_uart=1

# for Camera
dtoverlay=ov5647
start_x=1
gpu_mem=128

# /boot/firmware/config.txt (Bookworm) or /boot/config.txt (older)
camera_auto_detect=1
# or explicitly:
dtoverlay=ov5647

# Enable 64-bit booting (required!)
arm_64bit=1

# Enable UART for serial console
enable_uart=1

# Force HDMI output
hdmi_force_hotplug=1

# Use KMS graphics and set GPU memory
dtoverlay=vc4-kms-v3d
gpu_mem=128

# Tell firmware which DTB to load (optional if using default)
device_tree=bcm2711-rpi-4-b.dtb

# Kernel file to load (default is kernel8.img — but we override this)
kernel=Image
```

boot messages on HDMI:

```
# config.txt
enable_uart=1
hdmi_force_hotplug=1
```



# qt working application on vnc

```sh
QT_QPA_PLATFORM=vnc QT_QPA_VNC_PORT=5900 qt-example
```



# camera application on raspberrypi

**pc:**

```
nc -l -p 5000 | ffplay -fflags nobuffer -flags low_delay -framedrop -
```

**Raspberry pi:**

```sh
libcamera-vid -t 0 --inline -o - | nc 192.168.1.11 5000
```

