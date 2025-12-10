#ifndef SPIN_SHARED_H
#define SPIN_SHARED_H

#include <linux/spinlock.h>
#include <linux/types.h>

/*
 * Shared data between:
 *  - spin_hrtimer_demo (Module 1)
 *  - spin_kthreads_demo (Module 2)
 *  - spin_irq_demo      (Module 3)
 */
struct spin_demo_shared {
	spinlock_t lock;

	/* updated mainly by the hrtimer module */
	u64 timer_fires;
	u64 timer_work;

	/* updated mainly by the kthreads module */
	u64 kthread_counter;

	/* updated mainly by the IRQ module */
	u64 irq_count;
	u64 irq_last_ts_ns;
};

extern struct spin_demo_shared spin_demo_shared;

#endif /* SPIN_SHARED_H */
