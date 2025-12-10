#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>
#include <linux/sched.h>

static struct task_struct *worker1;
static struct task_struct *worker2;

static spinlock_t counter_lock;
static u64 shared_counter;

/* statistics */
static u64 max_cs_ns;      /* max critical section duration observed */
static u64 lock_failures;  /* trylock failures */

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
			got_lock = spin_trylock(&counter_lock);
			if (!got_lock) {
				lock_failures++;
				/* backoff a bit to increase contention pattern variety */
				cpu_relax();
				continue;
			}
		} else {
			spin_lock(&counter_lock);
			got_lock = true;
		}

		start_ns = ktime_get_ns();
		/* ------------ critical section ------------ */
		shared_counter++;
		/* simulate some small work */
		cpu_relax();
		/* ------------ end critical section -------- */
		end_ns = ktime_get_ns();

		duration = end_ns - start_ns;
		if (duration > max_cs_ns)
			max_cs_ns = duration;

		spin_unlock(&counter_lock);

		/*
		 * Add a small sleep every so often so the system stays responsive
		 * and to let the scheduler move threads around.
		 */
		if ((i & 0xFFF) == 0)
			cond_resched(); /* or msleep(1); */
	}

	pr_info("spin_kthreads_demo: %s exiting\n", name);
	return 0;
}

static int __init spin_kthreads_demo_init(void)
{
	int ret;

	pr_info("spin_kthreads_demo: init\n");

	spin_lock_init(&counter_lock);
	shared_counter = 0;
	max_cs_ns = 0;
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
	pr_info("spin_kthreads_demo: exit, stopping workers...\n");

	if (worker1)
		kthread_stop(worker1);
	if (worker2)
		kthread_stop(worker2);

	pr_info("spin_kthreads_demo: final shared_counter=%llu\n", shared_counter);
	pr_info("spin_kthreads_demo: max critical section duration=%lluns\n",
		max_cs_ns);
	pr_info("spin_kthreads_demo: lock_failures (trylock)=%llu\n",
		lock_failures);
}

module_init(spin_kthreads_demo_init);
module_exit(spin_kthreads_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ahmed + ChatGPT");
MODULE_DESCRIPTION("Spinlock demo with two kthreads and high-resolution timing");
