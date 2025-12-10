# Module 1: export the shared struct + use it

`spin_hrtimer_demo.c` Key points:

struct spin_demo_shared spin_demo_shared; is the definition.

EXPORT_SYMBOL_GPL(spin_demo_shared); exports it for other modules.

Both timer and kthreads will use spin_demo_shared.lock.


# Module 2: use the exported shared struct

`spin_kthreads_demo.c` :

Note: we do not define spin_demo_shared here; we just include the header and rely on the exported symbol from Module 1.


# How to build and run

```sh
make

# 1) Load Module 1 (exports spin_demo_shared + hrtimer)
sudo insmod spin_hrtimer_demo.ko interval_ns=5000000    # 5 ms

# 2) Load Module 2 (links against exported spin_demo_shared)
sudo insmod spin_kthreads_demo.ko loops_per_thread=200000 use_trylock=0

dmesg -w   # watch both modules logging

# When done:
sudo rmmod spin_kthreads_demo
sudo rmmod spin_hrtimer_demo
dmesg | tail -n 40
```

You should see:

From Module 1: messages that show fires, timer_work, and kthread_counter.

From Module 2: kthread_counter, cs_max, cs_avg, and optionally lock_failures (if use_trylock=1).

Now the same spinlock is:

Taken in hrtimer callback (softirq context, with spin_lock_irqsave).

Taken in two kthreads (process context, spin_lock / spin_trylock).

So you finally get true inter-module contention on a shared spinlock 👌
