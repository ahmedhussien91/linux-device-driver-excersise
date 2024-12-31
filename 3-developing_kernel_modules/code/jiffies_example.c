
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/delay.h>

#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

static int __init jiffies_example_init(void)
{
    pr_info( "Jiffies: %lu\n", jiffies);
    pr_info( "HZ: %d\n", HZ);
    pr_info( "Seconds since boot: %lu\n", jiffies / HZ);
        
    msleep(5000);
    jiffies=0;
    
    pr_info( "Jiffies: %lu\n", jiffies);
    pr_info( "HZ: %d\n", HZ);
    pr_info( "Seconds since boot: %lu\n", jiffies / HZ);
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