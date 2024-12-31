#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>

static DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);
static int condition = 0;

static int __init wait_event_timeout_example_init(void)
{
    long timeout = msecs_to_jiffies(5000); // 5 seconds timeout

    printk(KERN_INFO "Waiting for condition to be true or timeout... seconds=%d\n", jiffies / HZ);

    // Simulate a condition change after 3 seconds
    msleep(3000);
    //condition = 1;

    if (wait_event_timeout(my_wait_queue, condition != 0, timeout)) {
        printk(KERN_INFO "Condition met before timeout. seconds=%d\n", jiffies / HZ);
    } else {
        printk(KERN_INFO "Timeout occurred before condition was met. seconds=%d\n", jiffies/ HZ);
    }

    return 0;
}

static void __exit wait_event_timeout_example_exit(void)
{
    printk(KERN_INFO "Exiting wait_event_timeout_example module.\n");
}

module_init(wait_event_timeout_example_init);
module_exit(wait_event_timeout_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple example of wait_event_timeout usage");