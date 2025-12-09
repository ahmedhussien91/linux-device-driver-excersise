## 1️⃣ Cross-build modules on the PC instead of the Pi

Conceptually you need 3 things:

1. **Same kernel version & config as on the Pi**
2. **Kernel build tree / headers**
3. **Cross-compiler (aarch64 for RPi4)**

With Yocto you already have all of that.

### Step 1: Build kernel once with Yocto

On your PC in the Yocto build dir:

```
bitbake virtual/kernel
```

After that, Yocto will have:

- Kernel source:
   `tmp/work-shared/raspberrypi4/kernel-source`
- Kernel build tree (with .config, generated headers, etc.), something like:
   `tmp/work/raspberrypi4-poky-linux/linux-raspberrypi/<ver>/build`

(There might be a `build` symlink in that `linux-raspberrypi/<ver>/` dir.)

### Step 2: Install the Yocto SDK (toolchain)

Generate and install SDK (if you haven’t yet):

```
./bitbake_bb_rpi -h rpi -d sysv -t custom-image -c # build SDK (compiler)
```

Install the resulting `.sh` toolchain, then in a new shell:

```
. /opt/yocto/poky/5.0.8/environment-setup-cortexa72-poky-linux
```

After this, you have:

- `CC=aarch64-poky-linux-gcc`
- `CROSS_COMPILE=aarch64-poky-linux-`
- `ARCH=arm64` (usually already set by the env)

### Step 3: Minimal out-of-tree module project on the PC

On the PC (not on the Pi), create a directory, e.g. `~/kmods/mymod`:

**mymod.c**

```
#include <linux/module.h>
#include <linux/kernel.h>

static int __init mymod_init(void)
{
    pr_info("mymod: hello from PC-built module!\n");
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
MODULE_DESCRIPTION("Simple test module built on PC for RPi4");
```

**Makefile**

```
obj-m := mymod.o

# Point this to the Yocto kernel *build* directory
KDIR ?= /opt/yocto/tmp/work/raspberrypi4_64-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi-6.6.63+git/

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

Now in that shell where you sourced the SDK env:

```
make
```

This will:

- Use `ARCH` and `CROSS_COMPILE` from the Yocto SDK env
- Build `mymod.ko` for **aarch64**, matching your RPi4 kernel

### Step 4: Copy to Pi & test

```
scp mymod.ko root@<rpi-ip>:/tmp/
ssh root@<rpi-ip> "insmod /tmp/mymod.ko && dmesg | tail"
```

No compile on the Pi at all – only `insmod`/`rmmod`.

------

## 2️⃣ Making module development nicer on PC (plugins, tools, workflow)

You basically want:

- Good **code navigation** (go to definition, etc.)
- **Kernel headers** visible to the IDE
- A convenient **build command** (one click / one hotkey)
- Easy **deploy & test**

### 🧰 Recommended setup

**Editor:** VS Code (fits your usage really well)

1. **Install extensions:**

   - `C/C++` (Microsoft) – intellisense, navigation
   - Optionally: `clangd` + `clangd` extension if you prefer that engine
   - Optional but nice:
     - “Linux Kernel Dev” / “Linux Kernel” type extensions (for tags, symbol search)
        (names vary, but any that knows kernel/Kconfig helps)

2. **Point VS Code to the kernel build tree:**

   In your module folder, create `c_cpp_properties.json` (or use `compile_commands.json` if you’re fancy):

   - Add include paths like:
     - `${KERNEL_SRC}/include`
     - `${KERNEL_BUILD}/include`
     - `${KERNEL_SRC}/arch/arm64/include`

   Where:

   - `KERNEL_SRC = /opt/yocto/tmp/work/raspberrypi4_64-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi-6.6.63+git`
   - `KERNEL_BUILD = /opt/yocto/tmp/work/raspberrypi4_64-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi-6.6.63+git/`

   This gives you proper intellisense for kernel APIs.

3. **Add a build task in VS Code:**

   `tasks.json`:

   ```
   {
     "version": "2.0.0",
     "tasks": [
       {
         "label": "Build mymod",
         "type": "shell",
         "command": ". /opt/yocto/poky/5.0.8/environment-setup-cortexa72-poky-linux && export KDIR=\"/opt/yocto/tmp/work/raspberrypi4_64-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi-6.6.63+git\" && make",
         "problemMatcher": "$gcc"
       }
     ]
   }
   ```

   Then you can press `Ctrl+Shift+B` to build the module cross-compiled for RPi4.

4. **Optional: add a deploy & test script**

   Simple script `deploy_and_test.sh`:

   ```
   #!/usr/bin/env bash
   set -e

   TARGET=root@192.168.x.x

   scp mymod.ko $TARGET:/tmp/
   ssh $TARGET "rmmod mymod 2>/dev/null || true; insmod /tmp/mymod.ko; dmesg | tail -n 20"
   ```

   Call it from a VS Code task or manually — now your loop is:

   - Edit → `Ctrl+Shift+B` → `./deploy_and_test.sh`
