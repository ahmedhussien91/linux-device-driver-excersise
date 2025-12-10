# SPIN lock

## 1. Module 1 – High-resolution timer + spinlock

File: `spin_hrtimer_demo.c`

This module:

sets up an hrtimer (high-resolution timer) using **CLOCK_MONOTONIC**

fires periodically (configurable ns interval)

uses a spinlock + irqsave in the hrtimer callback (runs in softirq context)

updates shared counters under the lock

## How to play with it

```sh
# insert with default 20 ms
sudo insmod spin_hrtimer_demo.ko

# or e.g. 5ms
sudo insmod spin_hrtimer_demo.ko interval_ns=5000000

dmesg -w
# watch logs, you should see lines every 100 fires

sudo rmmod spin_hrtimer_demo
dmesg | tail
```

### This already shows:

hrtimer usage

spin_lock_irqsave/spin_unlock_irqrestore inside a timer callback (softirq context)

basic statistics protected by a spinlock

# Module 2 – Two kthreads contending on a spinlock

File: `spin_kthreads_demo.c`

This module:

creates two kernel threads

both threads contend on the same spinlock around a shared counter

uses ktime_get_ns() as a high-resolution time source to measure how long they stay in the critical section

shows different spinlock APIs:

- spin_lock

- spin_trylock

(and can easily be changed to spin_lock_irqsave if you later access from IRQ context)


## How to play with it:

```sh
# Simple case: blocking spin_lock()
sudo insmod spin_kthreads_demo.ko loops_per_thread=200000 use_trylock=0
dmesg -w
sudo rmmod spin_kthreads_demo

# Try spin_trylock variant
sudo insmod spin_kthreads_demo.ko loops_per_thread=200000 use_trylock=1
dmesg -w
sudo rmmod spin_kthreads_demo
```

### Look at:

shared_counter – should be close to 2 * loops_per_thread

max critical section duration – how long the CS ever took (ns)

lock_failures – how often spin_trylock failed
