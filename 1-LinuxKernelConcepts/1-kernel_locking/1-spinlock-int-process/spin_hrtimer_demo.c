#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>

static unsigned long interval_ns = 20 * 1000 * 1000; /* 20 ms default */
module_param(interval_ns, ulong, 0644);
MODULE_PARM_DESC(interval_ns, "Timer period in nanoseconds");

static struct hrtimer my_timer;
static ktime_t period;

static spinlock_t stats_lock;
static u64 timer_fires;
static u64 work_counter;

static u64 max_cs_ns;
static u64 total_cs_ns;
static u64 cs_count;

static enum hrtimer_restart my_timer_callback(struct hrtimer *t)
{
    unsigned long flags;
    u64 start_ns, end_ns, duration;
    u64 now_ns = ktime_to_ns(ktime_get());

    /* measure from just before taking the lock to just after releasing it */
    start_ns = ktime_get_ns();

    spin_lock_irqsave(&stats_lock, flags); // disable interrupts while holding the lock, to avoid deadlocks
    timer_fires++; // increment timer fire count
    work_counter += 10; // simulate some work
    spin_unlock_irqrestore(&stats_lock, flags); // restore interrupt state

    end_ns = ktime_get_ns();
    duration = end_ns - start_ns;

    /* update stats without extra locking: stats_lock already protects them */
    if (duration > max_cs_ns)
        max_cs_ns = duration;
    total_cs_ns += duration;
    cs_count++;

    if ((timer_fires % 1000) == 0) {
        u64 avg = cs_count ? (total_cs_ns / cs_count) : 0;
        pr_info("spin_hrtimer_demo: fires=%llu work=%llu now=%llu ns cs_max=%lluns cs_avg=%lluns\n",
                timer_fires, work_counter, now_ns, max_cs_ns, avg);
    }

    hrtimer_forward_now(&my_timer, period);
    return HRTIMER_RESTART;
}

static int __init spin_hrtimer_demo_init(void)
{
	pr_info("spin_hrtimer_demo: init, period=%lu ns\n", interval_ns);

	spin_lock_init(&stats_lock); // initialize the spinlock

	period = ktime_set(0, interval_ns); // seconds, nanoseconds

	hrtimer_init(&my_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED); // use pinned mode to avoid migration, which can skew timing
	my_timer.function = my_timer_callback; // set the callback function

	timer_fires = 0;
	work_counter = 0;

	hrtimer_start(&my_timer, period, HRTIMER_MODE_REL_PINNED); // start the timer

	return 0;
}

static void __exit spin_hrtimer_demo_exit(void)
{
    unsigned long flags;
    u64 fires, work, max_cs, total_cs, count, avg;

    if (hrtimer_cancel(&my_timer))
        pr_info("spin_hrtimer_demo: timer was active, cancelled now\n");

    spin_lock_irqsave(&stats_lock, flags); // protect stats during readout
    fires   = timer_fires;
    work    = work_counter;
    max_cs  = max_cs_ns;
    total_cs = total_cs_ns;
    count   = cs_count;
    spin_unlock_irqrestore(&stats_lock, flags); // restore interrupt state

    avg = count ? (total_cs / count) : 0;

    pr_info("spin_hrtimer_demo: exit fires=%llu work=%llu cs_max=%lluns cs_avg=%lluns samples=%llu\n",
            fires, work, max_cs, avg, count);
}


module_init(spin_hrtimer_demo_init);
module_exit(spin_hrtimer_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ahmed + ChatGPT");
MODULE_DESCRIPTION("High-resolution timer demo using spin_lock_irqsave");
