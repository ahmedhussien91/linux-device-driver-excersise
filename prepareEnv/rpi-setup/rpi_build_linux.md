##### **1. Use `devtool modify` — your controlled playground**

`devtool modify` is EXTREMELY fast compared to editing Yocto recipes manually.

It creates a working copy of the kernel sources **outside tmp/**, stable, not deleted by bitbake.

**Run this once:**

```
devtool modify linux-raspberrypi
```

This gives you:

📌 **Workspace source directory:**
 `workspace/sources/linux-raspberrypi/`

📌 **Your bbappend:**
 `workspace/appends/linux-raspberrypi.bbappend`

##### **2. Edit kernel source directly like a normal Git repo**

You now treat this directory like your own kernel repo:

```
workspace/sources/linux-raspberrypi/
    |-- drivers/
    |-- arch/
    |-- include/
    |-- scripts/
    |-- Documentation/
```

Modify anything:

- C code
- DTS/DTBO
- Kconfig
- Makefiles

This is *your* playground.

##### **3. Rebuild JUST the kernel (super fast)**

```
devtool build linux-raspberrypi
```

This is faster than bitbake because:

- It does not rebuild rootfs
- It skips do_rootfs, do_image, do_package, etc.
- No dependency scanning of the entire distro

It only rebuilds kernel, modules, dtb.

##### **4. Deploy to Raspberry Pi instantly**

A) If the board is reachable via SSH:

```
devtool deploy-target linux-raspberrypi root@192.168.1.101
```

This copies:

- `Image`
- DTBs
- Overlays
- Kernel modules
   directly into the correct directories on the running filesystem.

After that, just reboot:

```
ssh root@192.168.1.101 reboot
```

#### 