#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/delay.h>

static int __init jiffies_example_init(void)
{
    printk(KERN_INFO "Jiffies: %lu\n", jiffies);
    printk(KERN_INFO "HZ: %d\n", HZ);
    printk(KERN_INFO "Seconds since boot: %lu\n", jiffies / HZ);
        
    // Set the task state to interruptible
    // set_current_state(TASK_INTERRUPTIBLE);

    // Delay for 5 seconds
    msleep(5000);
    
    printk(KERN_INFO "Jiffies: %lu\n", jiffies);
    printk(KERN_INFO "HZ: %d\n", HZ);
    printk(KERN_INFO "Seconds since boot: %lu\n", jiffies / HZ);
    return 0;
}

static void __exit jiffies_example_exit(void)
{
    printk(KERN_INFO "Exiting Jiffies Example.\n");
}

module_init(jiffies_example_init);
module_exit(jiffies_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Jiffies Example");
