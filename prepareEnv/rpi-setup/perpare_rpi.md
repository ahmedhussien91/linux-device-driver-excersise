# setup rpi image

## download and build yocto Image

1. download [ycoto-excersise](git@github.com:ahmedhussien91/ycoto-excersise.git)

2. ```
   ./bitbake_bb_rpi -h rpi -d sysv -t custom-image # build image
   ```

3. ```
   ./bitbake_bb_rpi -h rpi -d sysv -t custom-image -c # build SDK (compiler)
   ```

4. ```sh
   ./bitbake_bb_rpi -h rpi -d sysv -s # to have bitbake of this HW in use
   ```

5. flashing and building

   #### 1. flashing rpi image

   [rpi_flash_sd](rpi_flash_sd.md)

   #### 2. building linux

   [rpi_build_linux](rpi_build_linux.md)

   #### 3. build modules with SDK

   ```sh
   . /opt/yocto/poky/5.0.8/environment-setup-cortexa72-poky-linux
   ```

   [rpi_build_modules](rpi_build_modules.md)