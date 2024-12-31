#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>

struct task_struct *tasks;
static int __init my_module_init(void) {

    pr_info("my_module_init\n");

    rcu_read_unlock();
    for_each_process(tasks) {
        pr_info("Process: %s [PID: %d]\n", tasks->comm, tasks->pid);
    }
    rcu_read_lock();

    return 0;
}

static void __exit my_module_exit(void) {
    pr_info("Module unloaded.\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Iterate over processes using for_each_process");
