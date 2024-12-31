#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "my_device"
#define DEVICE_TYPE 'M' // Unique identifier for the device

// Define ioctl commands
#define IOCTL_READ  _IOR(DEVICE_TYPE, 1, int)       // Read an integer
#define IOCTL_WRITE _IOW(DEVICE_TYPE, 2, int)       // Write an integer
#define IOCTL_RDWR  _IOWR(DEVICE_TYPE, 3, struct my_data) // Read/Write struct


struct my_data {
    int val1;
    int val2;
};
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static long device_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops = {
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

static struct cdev c_dev;
static dev_t dev_num;

static int __init example_init(void) {
    if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) {
        return -1;
    }
    cdev_init(&c_dev, &fops);
    if (cdev_add(&c_dev, dev_num, 1) == -1) {
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }
    printk(KERN_INFO "Example device registered with major %d\n", MAJOR(dev_num));
    return 0;
}

static void __exit example_exit(void) {
    cdev_del(&c_dev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "Example device unregistered\n");
}

static int device_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Device closed\n");
    return 0;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int value;
    struct my_data data;

    switch (cmd) {
        case IOCTL_READ:
            value = 42; // Example: return 42
            if (copy_to_user((int __user *)arg, &value, sizeof(value))) {
                return -EFAULT;
            }
            break;

        case IOCTL_WRITE:
            if (copy_from_user(&value, (int __user *)arg, sizeof(value))) {
                return -EFAULT;
            }
            pr_info("Value written by user: %d\n", value);
            break;

        case IOCTL_RDWR:
            if (copy_from_user(&data, (struct my_data __user *)arg, sizeof(data))) {
                return -EFAULT;
            }
            pr_info("Data received: val1=%d, val2=%d\n", data.val1, data.val2);

            data.val1 += 10; // Modify the data
            data.val2 += 20;
            if (copy_to_user((struct my_data __user *)arg, &data, sizeof(data))) {
                return -EFAULT;
            }
            break;

        default:
            return -EINVAL;
    }

    return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example Author");
MODULE_DESCRIPTION("Example ioctl device driver");

module_init(example_init);
module_exit(example_exit);