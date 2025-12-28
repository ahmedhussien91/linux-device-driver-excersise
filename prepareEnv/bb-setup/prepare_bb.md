# Setup BeagleBone Black Image

## Download and Build Yocto Image

1. Download [yocto-exercise](git@github.com:ahmedhussien91/ycoto-excersise.git)

2. ```bash
   ./bitbake_bb_rpi -h bb -d sysv -t custom-image # build image
   ```

3. ```bash
   ./bitbake_bb_rpi -h bb -d sysv -t custom-image -c # build SDK (compiler)
   ```

4. ```bash
   ./bitbake_bb_rpi -h bb -d sysv -s # to have bitbake of this HW in use
   ```

5. Flashing and Building

   #### 1. Network Boot Setup (Recommended)

   [bb_network_boot](bb_network_boot.md)

   #### 2. Flashing BeagleBone Image

   [bb_flash_sd](bb_flash_sd.md)

   #### 3. Building Linux

   [bb_build_linux](bb_build_linux.md)

   #### 4. Build Modules with SDK

   ```bash
   . /opt/yocto/poky/5.0.14/environment-setup-armv7at2hf-neon-poky-linux-gnueabi
   ```

   [bb_build_modules](bb_build_modules.md)
