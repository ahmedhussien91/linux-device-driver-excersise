#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include "spin_shared.h"

/*
 * Two kthreads increment spin_demo_shared.kthread_counter while contending
 * on spin_demo_shared.lock, which is also used by the hrtimer module.
 */

static struct task_struct *worker1;
static struct task_struct *worker2;

/* local stats for this module */
static u64 max_cs_ns;
static u64 total_cs_ns;
static u64 cs_count;
static u64 lock_failures;

static unsigned int loops_per_thread = 100000;
module_param(loops_per_thread, uint, 0644);
MODULE_PARM_DESC(loops_per_thread, "Number of iterations per worker thread");

static unsigned int use_trylock = 0;
module_param(use_trylock, uint, 0644);
MODULE_PARM_DESC(use_trylock, "Use spin_trylock instead of spin_lock (0/1)");

static int worker_thread_fn(void *data)
{
	const char *name = (const char *)data;
	unsigned int i;

	pr_info("spin_kthreads_demo: %s starting, loops=%u, use_trylock=%u\n",
		name, loops_per_thread, use_trylock);

	for (i = 0; i < loops_per_thread && !kthread_should_stop(); ++i) {
		u64 start_ns, end_ns, duration;
		bool got_lock = false;

		if (use_trylock) {
			got_lock = spin_trylock(&spin_demo_shared.lock);
			if (!got_lock) {
				lock_failures++;
				cpu_relax();
				continue;
			}
		} else {
			spin_lock(&spin_demo_shared.lock);
			got_lock = true;
		}

		start_ns = ktime_get_ns();

		/* ------------- critical section (shared with hrtimer) ------------ */
		spin_demo_shared.kthread_counter++;
		/* simulate tiny work */
		cpu_relax();
		/* --------- end critical section (shared with hrtimer) ------------ */

		end_ns = ktime_get_ns();
		duration = end_ns - start_ns;

		if (duration > max_cs_ns)
			max_cs_ns = duration;

		total_cs_ns += duration;
		cs_count++;

		spin_unlock(&spin_demo_shared.lock);

		if ((i & 0xFFF) == 0)
			cond_resched();
	}

	pr_info("spin_kthreads_demo: %s exiting\n", name);
	return 0;
}

static int __init spin_kthreads_demo_init(void)
{
	int ret;

	pr_info("spin_kthreads_demo: init\n");

	/*
	 * We assume spin_hrtimer_demo is already loaded and has initialized
	 * spin_demo_shared.lock. If not, insmod will fail at link time with
	 * "unknown symbol spin_demo_shared".
	 */

	max_cs_ns   = 0;
	total_cs_ns = 0;
	cs_count    = 0;
	lock_failures = 0;

	worker1 = kthread_run(worker_thread_fn, "worker1", "spin_worker1");
	if (IS_ERR(worker1)) {
		ret = PTR_ERR(worker1);
		pr_err("spin_kthreads_demo: failed to create worker1: %d\n", ret);
		return ret;
	}

	worker2 = kthread_run(worker_thread_fn, "worker2", "spin_worker2");
	if (IS_ERR(worker2)) {
		ret = PTR_ERR(worker2);
		pr_err("spin_kthreads_demo: failed to create worker2: %d\n", ret);
		kthread_stop(worker1);
		return ret;
	}

	return 0;
}

static void __exit spin_kthreads_demo_exit(void)
{
	u64 kthreads, max_cs, avg, count, fails;

	pr_info("spin_kthreads_demo: exit, stopping workers...\n");

	if (worker1)
		kthread_stop(worker1);
	if (worker2)
		kthread_stop(worker2);

	/* taking the lock is optional here, but shows it's still usable */
	spin_lock(&spin_demo_shared.lock);
	kthreads = spin_demo_shared.kthread_counter;
	spin_unlock(&spin_demo_shared.lock);

	max_cs = max_cs_ns;
	count  = cs_count;
	avg    = count ? (total_cs_ns / count) : 0;
	fails  = lock_failures;

	pr_info("spin_kthreads_demo: final kthread_counter=%llu cs_max=%lluns cs_avg=%lluns samples=%llu lock_failures=%llu\n",
		kthreads, max_cs, avg, count, fails);
}

module_init(spin_kthreads_demo_init);
module_exit(spin_kthreads_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ahmed + ChatGPT");
MODULE_DESCRIPTION("Spinlock demo: kthreads contending on shared lock exported by another module");
