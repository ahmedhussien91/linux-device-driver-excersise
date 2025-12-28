#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define GPIO_NUM 60   // GPIO1_28 → (1 * 32 + 28)

static int irq_number;

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    pr_info("BBB GPIO IRQ: Interrupt triggered!\n");
    return IRQ_HANDLED;
}

static int __init gpio_irq_init(void)
{
    int ret;

    pr_info("BBB GPIO IRQ: init\n");

    ret = gpio_request(GPIO_NUM, "bbb_gpio_irq");
    if (ret) {
        pr_err("Failed to request GPIO\n");
        return ret;
    }

    gpio_direction_input(GPIO_NUM);

    irq_number = gpio_to_irq(GPIO_NUM);
    if (irq_number < 0) {
        pr_err("Failed to get IRQ number\n");
        gpio_free(GPIO_NUM);
        return irq_number;
    }

    ret = request_irq(irq_number,
                      gpio_irq_handler,
                      IRQF_TRIGGER_FALLING,
                      "bbb_gpio_irq",
                      NULL);

    if (ret) {
        pr_err("Failed to request IRQ\n");
        gpio_free(GPIO_NUM);
        return ret;
    }

    pr_info("GPIO %d mapped to IRQ %d\n", GPIO_NUM, irq_number);
    return 0;
}

static void __exit gpio_irq_exit(void)
{
    pr_info("BBB GPIO IRQ: exit\n");
    free_irq(irq_number, NULL);
    gpio_free(GPIO_NUM);
}

module_init(gpio_irq_init);
module_exit(gpio_irq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ahmed");
MODULE_DESCRIPTION("BeagleBone Black GPIO Interrupt Demo");
MODULE_VERSION("1.0");
