#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))

/* --- PRCM / CM_PER --- */
#define CM_PER_BASE             0x44E00000u  /* :contentReference[oaicite:9]{index=9} */
#define CM_PER_GPIO1_CLKCTRL    (CM_PER_BASE + 0xACu) /* :contentReference[oaicite:10]{index=10} */

/* --- CONTROL_MODULE (pinmux) --- */
/* conf_gpmc_ben1 offset = 0x878 داخل CONTROL_MODULE :contentReference[oaicite:11]{index=11} */
#define CONTROL_MODULE_BASE     0x44E10000u
#define CONF_GPMC_BEN1          (CONTROL_MODULE_BASE + 0x878u)

/* --- GPIO1 --- */
#define GPIO1_BASE              0x4804C000u  /* :contentReference[oaicite:12]{index=12} */
#define GPIO_OE                 (GPIO1_BASE + 0x134u)
#define GPIO_DATAOUT            (GPIO1_BASE + 0x13Cu)
#define GPIO_SETDATAOUT         (GPIO1_BASE + 0x194u)
#define GPIO_CLEARDATAOUT       (GPIO1_BASE + 0x190u)

#define GPIO1_28                (1u << 28)

/* --- WDT (لمعطّل الـ Watchdog Timer) --- */
#define WDT_BASE   0x44E35000

#define WDT_WSPR   (*(volatile unsigned int *)(WDT_BASE + 0x48))
#define WDT_WWPS   (*(volatile unsigned int *)(WDT_BASE + 0x34))

static void wdt_disable(void)
{
    WDT_WSPR = 0xAAAA;
    while (WDT_WWPS & (1 << 4));

    WDT_WSPR = 0x5555;
    while (WDT_WWPS & (1 << 4));
}

/* بسيط: delay loop */
static void delay(volatile uint32_t n) {
    while (n--) { __asm__ volatile ("nop"); }
}

static void gpio1_enable_clock(void) {
    /* MODULEMODE = 0b10 (enable) في CM_PER_GPIO1_CLKCTRL
       (بنحافظ على باقي البتات) */
    uint32_t v = REG32(CM_PER_GPIO1_CLKCTRL);
    v &= ~0x3u;
    v |=  0x2u;
    REG32(CM_PER_GPIO1_CLKCTRL) = v;

    /* انتظر لحد ما يبقى enabled (IDLEST = 0) عادة في bits [17:16] */
    while (((REG32(CM_PER_GPIO1_CLKCTRL) >> 16) & 0x3u) != 0x0u) { }
}

static void pinmux_p9_12_to_gpio(void) {
    /* نموذج شائع:
       - MODE = 7 (bits [2:0]) => GPIO
       - PULLUP/DOWN, RXACTIVE حسب احتياجك
       هنا: MODE7 فقط، والباقي 0 (no rx). */
    REG32(CONF_GPMC_BEN1) = 0x7u;
}

static void gpio1_28_output(void) {
    /* OE: 0=output, 1=input */
    REG32(GPIO_OE) &= ~GPIO1_28;
}

int main(void) {
    wdt_disable();
    gpio1_enable_clock();
    pinmux_p9_12_to_gpio();
    gpio1_28_output();

    while (1) {
        REG32(GPIO_SETDATAOUT) = GPIO1_28;
        delay(500000000);
        REG32(GPIO_CLEARDATAOUT) = GPIO1_28;
        delay(500000000);
    }
}
