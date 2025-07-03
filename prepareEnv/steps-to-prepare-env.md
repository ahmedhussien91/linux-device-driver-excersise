# steps 

### Install dep libraries

```sh
sudo apt install libelf-dev
sudo apt install libssl-dev
sudo apt install libgmp3-dev
sudo apt install libmpc-dev
```

### Build linux

- Download and Checkout new Linux version 

  ```sh
  git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
  git checkout linux-6.7.y
  ```

- Cross-compile Linux 

  ```sh
  export PATH=/home/dell/Desktop/Linux_course/precompiled-toolchain/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-linux-gnueabihf/bin:$PATH
  export CROSS_COMPILE=arm-none-linux-gnueabihf-
  export ARCH=arm
  make vexpress_defconfig
  make menuconfig # Networking support -> Networking options -> 802.1Q/802.ad VLAN Support <M>
  # build linux 
  make
  # for beaglebone use /home/ahmed/Documents/Linux_course/Linux-yocto-Excersises/linux/code [https://github.com/ahmedhussien91/Linux-yocto-Excersises]
  # cd /home/ahmed/Documents/Linux_course/Linux-yocto-Excersises/linux/code/
  # . setenv_crossCompiler.sh
  # cd bb/linux # [./linux_build.sh]
  # make multi_v7_defconfig
  # make -j4 zImage
  # make -j4 dtbs
  # make modules
  # make INSTALL_MOD_PATH=/srv/nfs4/bb_busybox modules_install

  ```


### Setup Root File System


- Download files system [link](https://drive.google.com/file/d/10Cspz6olJ7rv5U-kvF0RDNhsX3_bQkIh/view?usp=drive_link)

- create drive to be used by qemu


  - ```sh
    # create memory to be used later
    dd if=/dev/zero of=rootfs.ext4 bs=1M count=1024
    mkfs.ext4 rootfs.ext4
    # mount file to /mnt folder
    sudo mount rootfs.ext4 /mnt
    # copy rootfilesystem downloaded files to /mnt  
    sudo cp -a rootfs/* /mnt/
    # sudo umount /mnt
    ```

- build and insert modules in files system 

  ```sh
  # From Linux Source  
  # build Modules
  make modules
  # install modules in the rootFS
  make INSTALL_MOD_PATH=/mnt modules_install
  ```

### Start Qemu


- Start qemu 

  - ```sh
    sudo qemu-system-arm -M vexpress-a9 -m 128M -nographic \
    -kernel /home/dell/Desktop/Linux_course/Linux-yocto-Excersises/linux/code/qemu/linux/arch/arm/boot/zImage \
    -append "console=ttyAMA0 root=/dev/vda rw" \
    -dtb /home/dell/Desktop/Linux_course/Linux-yocto-Excersises/linux/code/qemu/linux/arch/arm/boot/dts/arm/vexpress-v2p-ca9.dtb \
    -drive if=none,file=rootfs.ext4,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0
    ```

### Building your first module

create `hello.c` file 

```c
// SPDX-License-Identifier: GPL-2.0
/* hello.c */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
static int __init hello_init(void)
{
pr_alert("Good morrow to this fair assembly.\n");
return 0;
}
static void __exit hello_exit(void)
{
pr_alert("Alas, poor world, what treasure hast thou lost!\n");
}
module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Greeting module");
MODULE_AUTHOR("William Shakespeare");
```

create `makefile`

```makefile
# print the value of PROGS
ifneq ($(KERNELRELEASE),)
obj-m := $(PROGS)
else
KDIR := /home/dell/Desktop/Linux_course/Linux-yocto-Excersises/linux/code/bb/linux
# Source files
SRCS := $(wildcard *.c)
# Module object files
PROGS := $(SRCS:.c=.o)
all:
	$(MAKE) -C $(KDIR) M=$$PWD PROGS="$(PROGS)"
endif

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

# Target to print the values of SRCS and PROGS
print-vars:
	@echo "SRCS = $(SRCS)"
	@echo "PROGS = $(PROGS)"
```

copy `hello.ko` to kernel root file system in `/mnt`

```sh 
cp hello.ko /mnt/
```

From qemu insert the module to run withing the Linux image 

```sh
insmod hello.ko
```
