#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>


static struct timer_list my_timer;

void print_time_and_jiffies(struct timer_list *t)
{
    struct timespec64 time;
    struct tm tm;
    ktime_get_real_ts64(&time);
    time64_to_tm(time.tv_sec, 0, &tm);

    printk(KERN_INFO "Current local time: %04ld-%02d-%02d %02d:%02d:%02d\n",
           tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
           tm.tm_hour, tm.tm_min, tm.tm_sec);

    printk(KERN_INFO "Current time: %lu.%06lu seconds\n", time.tv_sec, time.tv_nsec);
    printk(KERN_INFO "Jiffies: %lu\n", jiffies);

    // Re-arm the timer to fire again in 5 seconds
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
}

static int __init timer_example_init(void)
{
    printk(KERN_INFO "Initializing timer example module.\n");

    // Initialize the timer
    timer_setup(&my_timer, print_time_and_jiffies, 0);

    // Set the timer to fire in 5 seconds
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));

    return 0;
}

static void __exit timer_example_exit(void)
{
    printk(KERN_INFO "Exiting timer example module.\n");

    // Remove the timer
    del_timer(&my_timer);
}

module_init(timer_example_init);
module_exit(timer_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple example of using a kernel timer to print time and jiffies cyclically");