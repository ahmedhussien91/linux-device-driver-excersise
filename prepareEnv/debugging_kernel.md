# Debug in linux using qemu

- start qemu and stop at the start by using `-s -S`

  - `-s`: Shorthand for `-gdb tcp::1234`, which sets up a GDB server on TCP port 1234.
    `-S`: Tells QEMU to stop the CPU at startup (**freezes the emulation**), allowing you to connect with GDB before the code starts running.

  - ```sh
    sudo qemu-system-arm -M vexpress-a9 -m 128M -nographic \
    -kernel /home/dell/Desktop/Linux_course/Linux-yocto-Excersises/linux/code/qemu/linux/arch/arm/boot/zImage \
    -append "console=ttyAMA0 root=/dev/vda rw" \
    -dtb /home/dell/Desktop/Linux_course/Linux-yocto-Excersises/linux/code/qemu/linux/arch/arm/boot/dts/arm/vexpress-v2p-ca9.dtb \
    -drive if=none,file=rootfs.ext4,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -s -S
    ```

- Start gdb remote debugging in **linux code folder** 

  - ```sh
    # sudo apt install gdb-multiarch
    gdb-multiarch vmlinux
    (gdb) target remote localhost:1234
    (gdb) b start_kernel
    (gdb) continue
    ```

  

## **Remote Debugging with GDB on beaglebone**

see [link](https://sergioprado.blog/debugging-the-linux-kernel-with-gdb/) & https://www.kernel.org/doc/html/v6.7/dev-tools/kgdb.html & check also in Linux repo `Documentation/dev-tools/kgdb.rst` & `Documentation/dev-tools/gdb-kernel-debugging.rst`

### **Configure the kernel with KGDB support**

To use KGDB, it is necessary to build the kernel with a few configuration options **enabled**:

- *CONFIG_KGDB*: this is the main option to enable KGDB support.
- *CONFIG_KGDB_SERIAL_CONSOLE*: enables the KGDB serial port driver.
- *CONFIG_MAGIC_SYSRQ*: enables the *Magic SysRq key* feature, used to enter the kernel debugger (i.e. start KGDB).
- *CONFIG_DEBUG_INFO*: compiles the kernel with debugging symbols.
- *CONFIG_FRAME_POINTER*: frame pointers improve the debugging experience (this option is usually enabled by default if supported by the architecture).

Also, it is recommended to disable a few configuration options:

- *CONFIG_DEBUG_INFO_REDUCED*: this should be disabled so debugging information for structure types is generated in the kernel ELF image.
- *CONFIG_RANDOMIZE_BASE*: if the architecture you are working on supports KASLR (Kernel Address Space Layout Randomization), you might want to disable this option. KASLR is a feature that enhances security by randomizing the memory address layout of the kernel but can affect the debugging experience. Alternatively, you can also pass *nokaslr* as a boot parameter to the Linux kernel.

#### Confirm that KGDB is enabled

At runtime, you can confirm if KGDB is available by checking for the existence of the `/sys/module/kgdboc` directory:

```sh
ls /sys/module/kgdboc/
```

> parameters  uevent

### Start KGDP

#### runtime within the image in shell

On Target

```sh
echo ttyS0 > /sys/module/kgdboc/parameters/kgdboc 
```

> [   15.733994] KGDB: Registered I/O driver kgdboc

```sh
echo g > /proc/sysrq-trigger
```

> [   28.708009] sysrq: DEBUG
> [   28.710921] KGDB: Entering KGDB

On Host

```sh
# source ../../setenv_crossCompiler.sh bb 
arm-cortex_a8-linux-gnueabihf-gdb vmlinux -tui
(gdb) target remote /dev/ttyUSB0
```

Make sure that you close the console terminal running on `/dev/ttyUSB0`

For making two instances of `/dev/ttyUSB0` use 

##### share the serial between console and gdb

There is one such tool provided by the Linux kernel community called ***kdmx***:

```sh
$ git clone https://kernel.googlesource.com/pub/scm/utils/kernel/kgdb/agent-proxy
$ cd agent-proxy/kdmx/
$ make
gcc -Wall -Wunreachable-code -D_XOPEN_SOURCE -c -o kdmx.o kdmx.c
gcc -o kdmx kdmx.o
```

This program will open the serial port and create two pseudo tty’s, one for the console and the other for KGDB:

```sh
$ ./kdmx -p /dev/ttyUSB0 -b 115200
/dev/pts/8 is slave pty for terminal emulator
/dev/pts/9 is slave pty for gdb
```

#### at linux booting time 

1. Add `kgdboc=ttyS0,115200 kgdbwait` to kernel `bootargs` in bootloader.

2. Connect to the board using GDB on the host:

```bash
arm-linux-gnueabihf-gdb vmlinux -tui
target remote /dev/ttyUSB0
```

##### not working on beaglebone

timeout in connection, check



### KDB

KDB provides a command-line interface to KGDB, making it possible to debug the kernel (at the assembly level) without a remote connection!

To use KDB, we just have to compile the kernel with *CONFIG_KGDB_KDB* enabled.

Then, when entering the kernel debugger, the KDB command-line interface will automatically be displayed in the console:

```plaintext
[0]kdb>
```

The *help* command can be executed to list all available commands:

```plaintext
[3]kdb> help
Command         Usage                Description
----------------------------------------------------------
md              <vaddr>             Display Memory Contents, also mdWcN, e.g. md8c1
mdr             <vaddr> <bytes>     Display Raw Memory
mdp             <paddr> <bytes>     Display Physical Memory
mds             <vaddr>             Display Memory Symbolically
mm              <vaddr> <contents>  Modify Memory Contents
go              [<vaddr>]           Continue Execution
rd                                  Display Registers
rm              <reg> <contents>    Modify Registers
ef              <vaddr>             Display exception frame
bt              [<vaddr>]           Stack traceback
btp             <pid>               Display stack for process <pid>
bta             [<state_chars>|A]   Backtrace all processes whose state matches
btc                                 Backtrace current process on each cpu
btt             <vaddr>             Backtrace process given its struct task address
env                                 Show environment variables
set                                 Set environment variables
help                                Display Help Message
?                                   Display Help Message
cpu             <cpunum>            Switch to new cpu
kgdb                                Enter kgdb mode
ps              [<state_chars>|A]   Display active task list
pid             <pidnum>            Switch to another task
reboot                              Reboot the machine immediately
lsmod                               List loaded kernel modules
sr              <key>               Magic SysRq key
dmesg           [lines]             Display syslog buffer
defcmd          name "usage" "help" Define a set of commands, down to endefcmd
kill            <-signal> <pid>     Send a signal to a process
summary                             Summarize the system
per_cpu         <sym> [<bytes>] [<cpu>]
                                    Display per_cpu variables
grephelp                            Display help on | grep
bp              [<vaddr>]           Set/Display breakpoints
bl              [<vaddr>]           Display breakpoints
bc              <bpnum>             Clear Breakpoint
be              <bpnum>             Enable Breakpoint
bd              <bpnum>             Disable Breakpoint
ss                                  Single Step
dumpcommon                          Common kdb debugging
dumpall                             First line debugging
dumpcpu                             Same as dumpall but only tasks on cpus
```

You can use KDB to inspect memory, registers, list processes, display the logs, and even set breakpoints to stop in a certain location. KDB is not a source-level debugger, although you can set breakpoints and execute some basic kernel run control.

