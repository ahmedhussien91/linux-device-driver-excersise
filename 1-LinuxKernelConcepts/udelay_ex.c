#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/ktime.h>

static DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);
static int condition = 0;

static int __init wait_event_timeout_example_init(void)
{
        long timeout = msecs_to_jiffies(5000); // 5 seconds timeout

   ktime_t ktime = ktime_get();
    s64 ns1 = ktime_to_ns(ktime);

    // pr_info("Kernel time in nanoseconds: %lld\n", ns);

    // Simulate a condition change after a small delay using udelay
    udelay(10); // 1 millisecond delay
    condition = 1;

    if (wait_event_timeout(my_wait_queue, condition != 0, timeout)) {
        ktime_t ktime = ktime_get();
    s64 ns2 = ktime_to_ns(ktime);

    pr_info("Kernel time in nanoseconds: %lld\n %lld\n", ns1, ns2);
    } else {
        printk(KERN_INFO "Timeout occurred before condition was met. jiffies=%lu\n", jiffies);
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
MODULE_DESCRIPTION("A simple example of wait_event_timeout usage with udelay");