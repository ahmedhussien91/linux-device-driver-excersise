# Kernel Delays

Kernel delays are typically implemented using various techniques such as busy-wait loops, sleep functions, or hardware timers. 
Delays can be used for: 

1. **Synchronization**: Ensuring that multiple processes or threads access shared resources in a controlled manner to avoid race conditions and ensure data integrity.
2. **Timing**: Implementing precise timing mechanisms for tasks that need to be executed at specific intervals or after a certain period.
3. **Resource Management**: Allowing the kernel to manage system resources efficiently by introducing delays when resources are not immediately available.
4. **Power Management**: Reducing power consumption by putting the system into a low-power state during idle periods.

When implementing kernel delays, it is important to consider the following:

- **Precision**: Ensure that the delays are precise enough for the intended purpose, especially in real-time systems.
- **Overhead**: Minimize the overhead introduced by delays to avoid degrading system performance.
- **Scalability**: Ensure that the delay mechanism scales well with the number of processes or threads in the system.
- **Power Consumption**: Optimize delays to reduce power consumption, especially in battery-powered devices.

By carefully managing kernel delays, developers can create more efficient and reliable systems.
you can use `msleep()` and `wait_event_timeout()` to wait in the kernel 

## Sleep functions `msleep()`

This example demonstrates how to use `msleep()` to create a delay in the kernel. It prints the current jiffies and HZ value before and after a 5-second sleep.

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/delay.h>

// Module initialization function
static int __init jiffies_example_init(void)
{
    printk(KERN_INFO "Jiffies: %lu\n", jiffies);
    printk(KERN_INFO "HZ: %d\n", HZ);
    printk(KERN_INFO "Seconds since boot: %lu\n", jiffies / HZ);
        
    msleep(5000); // Sleep for 5 seconds
    
    printk(KERN_INFO "Jiffies: %lu\n", jiffies);
    printk(KERN_INFO "HZ: %d\n", HZ);
    printk(KERN_INFO "Seconds since boot: %lu\n", jiffies / HZ);
    return 0;
}

// Module cleanup function
static void __exit jiffies_example_exit(void)
{
    printk(KERN_INFO "Exiting Jiffies Example.\n");
}

module_init(jiffies_example_init);
module_exit(jiffies_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Jiffies Example");
```

Example to demonstrate the `wait_event_timeout()`

```c
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>

static DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);
static int condition = 0;

// Module initialization function
static int __init wait_event_timeout_example_init(void)
{
    long timeout = msecs_to_jiffies(5000); // 5 seconds timeout

    printk(KERN_INFO "Waiting for condition to be true or timeout... seconds=%d\n", jiffies / HZ);

    // Simulate a condition change after 3 seconds
    msleep(3000);
    condition = 1;

    if (wait_event_timeout(my_wait_queue, condition != 0, timeout)) {
        printk(KERN_INFO "Condition met before timeout. seconds=%d\n", jiffies / HZ);
    } else {
        printk(KERN_INFO "Timeout occurred before condition was met. seconds=%d\n", jiffies / HZ);
    }

    return 0;
}

// Module cleanup function
static void __exit wait_event_timeout_example_exit(void)
{
    printk(KERN_INFO "Exiting wait_event_timeout_example module.\n");
}

module_init(wait_event_timeout_example_init);
module_exit(wait_event_timeout_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple example of wait_event_timeout usage");
```

### Short Delays with mdelay(), udelay() and CPU2() "Busy Waiting"
The `udelay()` function is a part of the Linux kernel's delay functions, which provide busy-wait loops for short delays. Here's a detailed explanation:

- **Usage**: It is typically used when precise short delays are needed, such as in hardware timing or when waiting for a short period before checking a condition again.

#### How `udelay()` Works

- **Busy-Wait Loop**: `udelay()` implements a busy-wait loop, meaning it keeps the CPU busy doing nothing but waiting for the specified time to pass. This is different from sleeping, where the CPU can perform other tasks.
- **Microsecond Precision**: The delay is specified in microseconds, allowing for very fine-grained control over timing. For example, `udelay(100)` introduces a delay of 100 microseconds (0.1 milliseconds).

### Example in Context

In the provided code, `udelay(100)` is used to introduce a 100-microsecond delay before setting the `condition` variable to 1. This ensures that the condition is met almost immediately after the delay, allowing the `wait_event_timeout` function to proceed without waiting for the full timeout period.

This precise short delay is crucial in scenarios where timing needs to be controlled very accurately, which is not possible using jiffies due to their coarser resolution.

```c
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>

static DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);
static int condition = 0;

// Module initialization function
static int __init wait_event_timeout_example_init(void)
{
    long timeout = msecs_to_jiffies(5000); // 5 seconds timeout

    printk(KERN_INFO "Waiting for condition to be true or timeout... jiffies=%lu\n", jiffies);

    // Simulate a condition change after a small delay using udelay
    udelay(100); // 1 millisecond delay
    condition = 1;

    if (wait_event_timeout(my_wait_queue, condition != 0, timeout)) {
        printk(KERN_INFO "Condition met before timeout. jiffies=%lu\n", jiffies);
    } else {
        printk(KERN_INFO "Timeout occurred before condition was met. jiffies=%lu\n", jiffies);
    }

    return 0;
}

// Module cleanup function
static void __exit wait_event_timeout_example_exit(void)
{
    printk(KERN_INFO "Exiting wait_event_timeout_example module.\n");
}

module_init(wait_event_timeout_example_init);
module_exit(wait_event_timeout_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple example of wait_event_timeout usage with udelay");
```

# kernel Timers

Linux kernel timers are mechanisms that allow you to schedule the execution of functions at a specific time in the future or periodically. They are useful for tasks that need to be performed after a delay or at regular intervals. Here are some key points about Linux kernel timers:

1. **Timer Initialization**: Timers are initialized using functions like `timer_setup()` which sets up the timer and associates it with a callback function that will be executed when the timer expires.

2. **Setting the Timer**: The timer is set to expire after a certain period using functions like `mod_timer()`. This function takes the timer and the expiration time (in jiffies) as arguments.

3. **Callback Function**: When the timer expires, the associated callback function is executed. This function can perform any necessary tasks and can also re-arm the timer if periodic execution is required.

4. **Timer Removal**: Timers can be removed using `del_timer()` to ensure they do not fire after the module is unloaded.

Overall, kernel timers are a powerful tool for managing time-based tasks within the Linux kernel.


**Example Usage**: The provided code example demonstrates a kernel module that initializes a timer to print the current time and jiffies every 5 seconds.
`timer_example.c`
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

static struct timer_list my_timer;

// Timer callback function
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

// Module initialization function
static int __init timer_example_init(void)
{
    printk(KERN_INFO "Initializing timer example module.\n");

    // Initialize the timer
    timer_setup(&my_timer, print_time_and_jiffies, 0);

    // Set the timer to fire in 5 seconds (passed in jiffies)
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));

    return 0;
}

// Module cleanup function
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
```

