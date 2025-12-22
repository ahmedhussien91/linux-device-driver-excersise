## 1️⃣ Cross-build modules on the PC for BeagleBone Black

Conceptually you need 3 things:

1. **Same kernel version & config as on the BeagleBone**
2. **Kernel build tree / headers**
3. **Cross-compiler (ARM for BeagleBone Black)**

With Yocto you already have all of that.

### Step 1: Build kernel once with Yocto

On your PC in the Yocto build dir:

```bash
bitbake virtual/kernel
```

After that, Yocto will have:

- Kernel source:
   `tmp/work-shared/beaglebone-yocto/kernel-source`
- Kernel build tree (with .config, generated headers, etc.), something like:
   `tmp/work/beaglebone_yocto-poky-linux-gnueabi/linux-yocto/<ver>/build`

(There might be a `build` symlink in that `linux-yocto/<ver>/` dir.)

### Step 2: Install the Yocto SDK (toolchain)

Generate and install SDK (if you haven't yet):

```bash
./bitbake_bb_rpi -h bb -d sysv -t custom-image -c # build SDK (compiler)
```

Install the resulting `.sh` toolchain, then in a new shell:

```bash
. /opt/yocto/poky/5.0.14/environment-setup-armv7at2hf-neon-poky-linux-gnueabi
```

After this, you have:

- `CC=arm-poky-linux-gnueabi-gcc`
- `CROSS_COMPILE=arm-poky-linux-gnueabi-`
- `ARCH=arm` (usually already set by the env)

### Step 3: Minimal out-of-tree module project on the PC

On the PC (not on the BeagleBone), create a directory, e.g. `~/kmods/mymod`:

**mymod.c**

```c
#include <linux/module.h>
#include <linux/kernel.h>

static int __init mymod_init(void)
{
    pr_info("mymod: hello from PC-built module for BeagleBone!\n");
    return 0;
}

static void __exit mymod_exit(void)
{
    pr_info("mymod: goodbye!\n");
}

module_init(mymod_init);
module_exit(mymod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ahmed");
MODULE_DESCRIPTION("Simple test module built on PC for BeagleBone Black");
```

**Makefile**

```makefile
obj-m := mymod.o

# Point this to the Yocto kernel *build* directory
KDIR ?= /opt/yocto/tmp/work/beaglebone-poky-linux-gnueabi/linux-bb.org/6.6.32+git/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
```

Now in that shell where you sourced the SDK env:

```bash
make
```

This will:

- Use `ARCH=arm` and `CROSS_COMPILE` from the Yocto SDK env
- Cross-compile against the BeagleBone kernel headers
- Produce `mymod.ko` compatible with your BeagleBone's kernel

### Step 4: Deploy the module to BeagleBone

**Option A: Via SCP (if networking is set up)**

```bash
scp mymod.ko root@192.168.1.101:/tmp/
ssh root@192.168.1.101 "insmod /tmp/mymod.ko"
ssh root@192.168.1.101 "dmesg | tail"
ssh root@192.168.1.101 "rmmod mymod"
```

**Option B: Via SD card**

1. Copy to SD card:
```bash
sudo cp mymod.ko /media/ahmed/rootfs/tmp/
```

2. Boot BeagleBone and load:
```bash
insmod /tmp/mymod.ko
dmesg | tail
rmmod mymod
```

### Step 5: More complex modules

For modules that need specific kernel features, make sure your kernel config includes them. You can check the current config:

```bash
# On the BeagleBone
zcat /proc/config.gz | grep CONFIG_FEATURE_NAME
```

Or during Yocto build:

```bash
# On the PC
bitbake virtual/kernel -c menuconfig
```

### Example: GPIO module for BeagleBone Black

**gpio_test.c**

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define GPIO_PIN 60  // P9_12 on BeagleBone Black

static int __init gpio_test_init(void)
{
    int ret;

    pr_info("GPIO Test: Initializing GPIO %d\n", GPIO_PIN);

    ret = gpio_request(GPIO_PIN, "gpio_test");
    if (ret) {
        pr_err("GPIO Test: Failed to request GPIO %d\n", GPIO_PIN);
        return ret;
    }

    gpio_direction_output(GPIO_PIN, 0);

    // Blink LED pattern
    gpio_set_value(GPIO_PIN, 1);
    msleep(1000);
    gpio_set_value(GPIO_PIN, 0);

    pr_info("GPIO Test: Module loaded successfully\n");
    return 0;
}

static void __exit gpio_test_exit(void)
{
    gpio_set_value(GPIO_PIN, 0);
    gpio_free(GPIO_PIN);
    pr_info("GPIO Test: Module unloaded\n");
}

module_init(gpio_test_init);
module_exit(gpio_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ahmed");
MODULE_DESCRIPTION("GPIO test module for BeagleBone Black");
```

### Debugging Tips

1. **Check kernel ring buffer**:
```bash
dmesg | tail -n 20
```

2. **View module info**:
```bash
modinfo mymod.ko
```

3. **List loaded modules**:
```bash
lsmod | grep mymod
```

4. **Check module dependencies**:
```bash
modprobe --show-depends mymod
```

5. **Verbose module loading**:
```bash
insmod mymod.ko dyndbg=+p
```

### Common Issues

1. **Version magic mismatch**: Ensure you're building against the exact same kernel version running on BeagleBone
2. **Missing symbols**: Check if required kernel features are enabled in config
3. **Architecture mismatch**: Verify you're using ARM cross-compiler, not x86_64
4. **Permission denied**: Make sure you're running as root when loading modules

### Advanced: Building with Device Tree Overlays

For BeagleBone-specific peripherals, you might need device tree overlays:

**example-overlay.dts**
```dts
/dts-v1/;
/plugin/;

/ {
    compatible = "ti,beaglebone", "ti,beaglebone-black";

    part-number = "example-overlay";
    version = "00A0";

    fragment@0 {
        target-path="/";
        __overlay__ {
            example_device {
                compatible = "example,device";
                status = "okay";
            };
        };
    };
};
```

Compile overlay:
```bash
dtc -O dtb -o example-overlay.dtbo -b 0 -@ example-overlay.dts
```
