#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include "spin_shared.h"

static unsigned long interval_ns = 20 * 1000 * 1000; /* 20 ms default */
module_param(interval_ns, ulong, 0644);
MODULE_PARM_DESC(interval_ns, "Timer period in nanoseconds");

static struct hrtimer my_timer;
static ktime_t period;

/* This is the actual definition; declared extern in spin_shared.h */
struct spin_demo_shared spin_demo_shared;

/* Export the whole struct symbol so other LKMs can use it */
EXPORT_SYMBOL_GPL(spin_demo_shared);

/* local timing stats for this module only */
static u64 max_cs_ns;
static u64 total_cs_ns;
static u64 cs_count;

static enum hrtimer_restart my_timer_callback(struct hrtimer *t)
{
	unsigned long flags;
	u64 start_ns, end_ns, duration;
	u64 now_ns = ktime_to_ns(ktime_get());

	/* measure the time spent in the critical section + lock ops */
	start_ns = ktime_get_ns();

	spin_lock_irqsave(&spin_demo_shared.lock, flags);

	/* shared data updated under the shared spinlock */
	spin_demo_shared.timer_fires++;
	spin_demo_shared.timer_work += 10;

	spin_unlock_irqrestore(&spin_demo_shared.lock, flags);

	end_ns = ktime_get_ns();
	duration = end_ns - start_ns;

	/* update local stats (no extra lock: we are in callback context) */
	if (duration > max_cs_ns)
		max_cs_ns = duration;

	total_cs_ns += duration;
	cs_count++;

	if ((spin_demo_shared.timer_fires % 1000) == 0) {
		u64 avg = cs_count ? (total_cs_ns / cs_count) : 0;
		pr_info("spin_hrtimer_demo: fires=%llu timer_work=%llu now=%llu ns cs_max=%lluns cs_avg=%lluns kthreads=%llu\n",
			spin_demo_shared.timer_fires,
			spin_demo_shared.timer_work,
			now_ns,
			max_cs_ns,
			avg,
			spin_demo_shared.kthread_counter);
	}

	hrtimer_forward_now(&my_timer, period);
	return HRTIMER_RESTART;
}

static int __init spin_hrtimer_demo_init(void)
{
	pr_info("spin_hrtimer_demo: init, period=%lu ns\n", interval_ns);

	/* Initialize shared struct and lock */
	spin_lock_init(&spin_demo_shared.lock);
	spin_demo_shared.timer_fires      = 0;
	spin_demo_shared.timer_work       = 0;
	spin_demo_shared.kthread_counter  = 0;

	max_cs_ns   = 0;
	total_cs_ns = 0;
	cs_count    = 0;

	period = ktime_set(0, interval_ns);

	hrtimer_init(&my_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
	my_timer.function = my_timer_callback;

	hrtimer_start(&my_timer, period, HRTIMER_MODE_REL_PINNED);

	return 0;
}

static void __exit spin_hrtimer_demo_exit(void)
{
	unsigned long flags;
	u64 fires, work, kthreads;
	u64 max_cs, total_cs, count, avg;

	if (hrtimer_cancel(&my_timer))
		pr_info("spin_hrtimer_demo: timer was active, cancelled now\n");

	/* Take the shared lock one last time to read consistent values */
	spin_lock_irqsave(&spin_demo_shared.lock, flags);
	fires    = spin_demo_shared.timer_fires;
	work     = spin_demo_shared.timer_work;
	kthreads = spin_demo_shared.kthread_counter;
	spin_unlock_irqrestore(&spin_demo_shared.lock, flags);

	max_cs   = max_cs_ns;
	total_cs = total_cs_ns;
	count    = cs_count;
	avg      = count ? (total_cs / count) : 0;

	pr_info("spin_hrtimer_demo: exit fires=%llu timer_work=%llu kthread_counter=%llu cs_max=%lluns cs_avg=%lluns samples=%llu\n",
		fires, work, kthreads, max_cs, avg, count);
}

module_init(spin_hrtimer_demo_init);
module_exit(spin_hrtimer_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ahmed + ChatGPT");
MODULE_DESCRIPTION("High-resolution timer demo exporting shared spinlock via EXPORT_SYMBOL_GPL");
