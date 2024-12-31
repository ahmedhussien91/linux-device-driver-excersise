# Linux Device Model



![14. The Linux Device Model - Linux Device Drivers, 3rd Edition [Book]](./assets/httpatomoreillycomsourceoreillyimages138413.png)



# MISC Driver



Kernel Driver 

```c
/*
 * ch1/miscdrv/miscdrv.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming (Part 2)"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming-Part-2
 *
 * From: Ch 1 : Writing a Simple Misc Device Driver
 ****************************************************************
 * Brief Description:
 * A simple 'skeleton' Linux character device driver, belonging to the
 * 'misc' class (major #10). Here, we simply build something of a 'template'
 * or skeleton for a misc driver.
 *
 * For details, please refer the book, Ch 1.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>		/* the fops, file data structures */
#include <linux/slab.h>
#include "../../convenient.h"

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("LKP-2 book:ch1/miscdrv: simple 'skeleton' misc char driver");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

/*
 * open_miscdrv()
 * The driver's open 'method'; this 'hook' will get invoked by the kernel VFS
 * when the device file is opened. Here, we simply print out some relevant info.
 * The POSIX standard requires open() to return the file descriptor on success;
 * note, though, that this is done within the kernel VFS (when we return). So,
 * all we do here is return 0 indicating success.
 * (The nonseekable_open(), in conjunction with the fop's llseek pointer set to
 * no_llseek, tells the kernel that our device is not seek-able).
 */
static int open_miscdrv(struct inode *inode, struct file *filp)
{
	char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

	if (unlikely(!buf))
		return -ENOMEM;
	PRINT_CTX();		// displays process (or atomic) context info

	pr_info(" opening \"%s\" now; wrt open file: f_flags = 0x%x\n",
		file_path(filp, buf, PATH_MAX), filp->f_flags);

	kfree(buf);
	return nonseekable_open(inode, filp);
}

/*
 * read_miscdrv()
 * The driver's read 'method'; it has effectively 'taken over' the read syscall
 * functionality! Here, we simply print out some info.
 * The POSIX standard requires that the read() and write() system calls return
 * the number of bytes read or written on success, 0 on EOF (for read) and -1 (-ve errno)
 * on failure; here we simply return 'count', pretending that we 'always succeed'.
 */
static ssize_t read_miscdrv(struct file *filp, char __user *ubuf, size_t count, loff_t *off)
{
	pr_info("(pretending) to read %zd bytes\n", count);
	return count;
}

/*
 * write_miscdrv()
 * The driver's write 'method'; it has effectively 'taken over' the write syscall
 * functionality! Here, we simply print out some info.
 * The POSIX standard requires that the read() and write() system calls return
 * the number of bytes read or written on success, 0 on EOF (for read) and -1 (-ve errno)
 * on failure; here we simply return 'count', pretending that we 'always succeed'.
 */
static ssize_t write_miscdrv(struct file *filp, const char __user *ubuf,
			     size_t count, loff_t *off)
{
	pr_info("(pretending) to write %zd bytes\n", count);
	return count;
}

/*
 * close_miscdrv()
 * The driver's close 'method'; this 'hook' will get invoked by the kernel VFS
 * when the device file is closed (technically, when the file ref count drops
 * to 0). Here, we simply print out some relevant info.
 * We simply return 0 indicating success.
 */
static int close_miscdrv(struct inode *inode, struct file *filp)
{
	char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

	if (unlikely(!buf))
		return -ENOMEM;
	pr_info("closing \"%s\"\n", file_path(filp, buf, PATH_MAX));
	kfree(buf);

	return 0;
}

static const struct file_operations llkd_misc_fops = {
	.open = open_miscdrv,
	.read = read_miscdrv,
	.write = write_miscdrv,
	.release = close_miscdrv,
	.llseek = no_llseek,
};

static struct miscdevice llkd_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,	/* kernel dynamically assigns a free minor# */
	.name = "llkd_miscdrv",	/* when misc_register() is invoked, the kernel
				 * will auto-create device file as /dev/llkd_miscdrv ;
				 * also populated within /sys/class/misc/ and /sys/devices/virtual/misc/ */
	.mode = 0666,			/* ... dev node perms set as specified here */
	.fops = &llkd_misc_fops,	/* connect to this driver's 'functionality' */
};

static int __init miscdrv_init(void)
{
	int ret;
	struct device *dev;

	ret = misc_register(&llkd_miscdev);
	if (ret != 0) {
		pr_notice("misc device registration failed, aborting\n");
		return ret;
	}

	/* Retrieve the device pointer for this device */
	dev = llkd_miscdev.this_device;
	pr_info("LLKD misc driver (major # 10) registered, minor# = %d,"
		" dev node is /dev/%s\n", llkd_miscdev.minor, llkd_miscdev.name);

	dev_info(dev, "sample dev_info(): minor# = %d\n", llkd_miscdev.minor);

	return 0;		/* success */
}

static void __exit miscdrv_exit(void)
{
	misc_deregister(&llkd_miscdev);
	pr_info("LLKD misc driver deregistered, bye\n");
}

module_init(miscdrv_init);
module_exit(miscdrv_exit);
```



User Space application to read and write to driver

```c
/*
 * ch1/miscdrv/rdwr_test.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming (Part 2)"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming-Part-2
 *
 * From: Ch 1 : Writing a Simple Misc Device Driver
 ****************************************************************
 * Brief Description:
 * USER SPACE app : a generic read-write test bed for demo drivers.
 * THis simple user mode app allows you to issue a read or a write
 * to a specified device file.
 *
 * For details, please refer the book, Ch 1.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define READ_OPT	'r'
#define WRITE_OPT	'w'

static int stay_alive;

static inline void usage(char *prg)
{
	fprintf(stderr, "Usage: %s r|w device_file num_bytes\n"
		" 'r' => read num_bytes bytes from the device node device_file\n"
		" 'w' => write num_bytes bytes to the device node device_file\n", prg);
}

int main(int argc, char **argv)
{
	int fd, flags = O_RDONLY;
	ssize_t n;
	char opt, *buf = NULL;
	size_t num = 0;

	if (argc != 4) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	opt = argv[1][0];
	if (opt != 'r' && opt != 'w') {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if (opt == WRITE_OPT)
		flags = O_WRONLY;

	fd = open(argv[2], flags, 0);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	printf("Device file \"%s\" opened (in %s mode): fd=%d\n",
	       argv[2], (flags == O_RDONLY ? "read-only" : "write-only"), fd);

	num = atoi(argv[3]);
	/* if ((num < 0) || (num > INT_MAX)) { */
	/* FYI, for the above line of code, the cppcheck(1) tool via the Makefile's
	 * 'sa' target caught this:
	 * "style: The unsigned expression 'num' will never be negative so it is either
	 * pointless or an error to check if it is. [unsignedLessThanZero]"
	 */
	if (num > INT_MAX) {
		fprintf(stderr, "%s: number of bytes '%zu' invalid.\n", argv[0], num);
		close(fd);
		exit(EXIT_FAILURE);
	}

	buf = calloc(num, 1);
	if (!buf) {
		fprintf(stderr, "%s: out of memory!\n", argv[0]);
		close(fd);
		exit(EXIT_FAILURE);
	}

	if (opt == READ_OPT) {	// test reading..
		n = read(fd, buf, num);
		if (n < 0) {
			perror("read failed");
			fprintf(stderr, "Tip: see kernel log\n");
			free(buf);
			close(fd);
			exit(EXIT_FAILURE);
		}
		printf("%s: read %zd bytes from %s\n", argv[0], n, argv[2]);
		printf(" Data read:\n\"%.*s\"\n", (int)n, buf);

#if 0
		/* Test the lseek; typically, it should fail */
		off_t ret = lseek(fd, 100, SEEK_CUR);
		if (ret == (off_t)-1)
			fprintf(stderr, "%s: lseek on device failed\n", argv[0]);
#endif
	} else {		// test writing ..
		n = write(fd, buf, num);
		if (n < 0) {
			perror("write failed");
			fprintf(stderr, "Tip: see kernel log\n");
			free(buf);
			close(fd);
			exit(EXIT_FAILURE);
		}
		printf("%s: wrote %zd bytes to %s\n", argv[0], n, argv[2]);
	}

	if (stay_alive == 1) {
		printf("%s:%d: stayin' alive (in pause()) ...\n", argv[0], getpid());
		pause();	/* block until a signal is received */
	}

	free(buf);
	close(fd);
	exit(EXIT_SUCCESS);
}
```



# Kernel Excersises

see [Kernel character device drivers Exercise](https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html)

## Major and Minor

In UNIX, the devices traditionally had a unique, fixed identifier associated with them.

The identifier consists of two parts: major and minor. 

1. Major identifies the device type (IDE disk, SCSI disk, serial port, etc.)

2. Minor identifies the device (first disk, second serial port, etc.).

## Data Structures for a character devices

In the kernel, a character-type device is represented by `struct cdev`, a structure used to register it in the system.

Drivers use 3 important structures:

1. `struct file_operations`
2. `struct file` 
3. `struct inode`.

### `struct file_operations`[¶](https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html#struct-file-operations)

The character device drivers receive unaltered system calls made by users over device-type files. Consequently, implementation of a character device driver means implementing the system calls specific to files: `open`, `close`, `read`, `write`, `lseek`, `mmap`, etc.

Operations are described in `struct file_operations`

```c
#include <linux/fs.h>

struct file_operations {
    struct module *owner;
    loff_t (*llseek) (struct file *, loff_t, int);
    ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
    [...]
    long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
    [...]
    int (*open) (struct inode *, struct file *);
    int (*flush) (struct file *, fl_owner_t id);
    int (*release) (struct inode *, struct file *);
    [...]
```

Most parameters for the presented operations have a direct meaning:

- `file` and `inode` identifies the device type file;
- `size` is the number of bytes to be read or written;
- `offset` is the displacement to be read or written (to be updated accordingly);
- `user_buffer` user buffer from which it reads / writes;
- `whence` is the way to seek (the position where the search operation starts);
- `cmd` and `arg` are the parameters sent by the users to the ioctl call (IO control).

#### `inode` and `file` structures[¶](https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html#inode-and-file-structures)

An `inode` represents a file from the point of view of the file system. Attributes of an inode are the size, rights, times associated with the file. An inode uniquely identifies a file in a file system.

The `file` structure is still a file, but closer to the user's point of view. From the attributes of the file structure we list: 

- the inode, 
- the file name, 
- the file opening attributes, 
- the file position. 

All open files at a given time have associated a `file` structure.

The file structure contains, among many fields:

- `f_mode`, which specifies read (`FMODE_READ`) or write (`FMODE_WRITE`);

 - `f_flags`, which specifies the file opening flags (`O_RDONLY`, `O_NONBLOCK`, `O_SYNC`, `O_APPEND`, `O_TRUNC`, etc.);
 - `f_op`, which specifies the operations associated with the file (pointer to the `file_operations` structure );
 - `private_data`, a pointer that can be used by the programmer to store device-specific data; The pointer will be initialized to a memory location assigned by the programmer.
 - `f_pos`, the offset within the file

The `inode` structure contains, among much information, an `i_cdev` field, which is a pointer to the structure that defines the character device (when the inode corresponds to a character device).

# Implementing character device

```c
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>


#define MY_MAJOR       42
#define MY_MAX_MINORS  5

struct my_device_data {
    struct cdev cdev;
    /* my data starts here */
    //...
};

static int my_open(struct inode *inode, struct file *file)
{
    struct my_device_data *my_data;

    my_data = container_of(inode->i_cdev, struct my_device_data, cdev);

    file->private_data = my_data;
    //...
}

static int my_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
    struct my_device_data *my_data;

    my_data = (struct my_device_data *) file->private_data;

    //...
}
struct my_device_data devs[MY_MAX_MINORS];

const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_release,
    .unlocked_ioctl = my_ioctl
};

int init_module(void)
{
    int i, err;

    err = register_chrdev_region(MKDEV(MY_MAJOR, 0), MY_MAX_MINORS,
                                 "my_device_driver");
    if (err != 0) {
        /* report error */
        return err;
    }

    for(i = 0; i < MY_MAX_MINORS; i++) {
        /* initialize devs[i] fields */
        cdev_init(&devs[i].cdev, &my_fops);
        cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, i), 1);
    }

    return 0;
}

void cleanup_module(void)
{
    int i;

    for(i = 0; i < MY_MAX_MINORS; i++) {
        /* release devs[i] fields */
        cdev_del(&devs[i].cdev);
    }
    unregister_chrdev_region(MKDEV(MY_MAJOR, 0), MY_MAX_MINORS);
}

static ssize_t my_write(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset)
{
    struct my_device_data *my_data;

    my_data = (struct my_device_data *) file->private_data;

    // Implement write functionality here
    // ...

    return size; // Return the number of bytes written
}

static int my_release(struct inode *inode, struct file *file)
{
    // Implement release functionality here
    // ...

    return 0; // Return success
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct my_device_data *my_data;

    my_data = (struct my_device_data *) file->private_data;

    // Implement ioctl functionality here
    // ...

    return 0; // Return success
}
```

