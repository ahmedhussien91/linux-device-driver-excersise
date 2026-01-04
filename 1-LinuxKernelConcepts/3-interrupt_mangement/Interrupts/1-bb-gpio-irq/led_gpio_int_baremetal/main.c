#include <stdint.h>
#include "uart.h"

#define REG32(a) (*(volatile uint32_t *)(a))

/* ========================= Teaching knobs ========================= */
#define TEACHING_POLL_ASSIST   0   /* 1 = allow polling assist path (NOT real IRQ), 0 = pure interrupt demo */
#define TEACHING_VERBOSE_DUMPS  1   /* 1 = dump registers after init */

/* -------- Base addresses -------- */
#define CM_PER_BASE        0x44E00000
#define CONTROL_BASE       0x44E10000
#define GPIO1_BASE         0x4804C000
#define AINTC_BASE         0x48200000

/* -------- GPIO registers -------- */
#define GPIO_OE            (GPIO1_BASE + 0x134)
#define GPIO_DATAIN        (GPIO1_BASE + 0x138)
#define GPIO_SETDATAOUT    (GPIO1_BASE + 0x194)
#define GPIO_CLEARDATAOUT  (GPIO1_BASE + 0x190)
#define GPIO_IRQSTATUS_0   (GPIO1_BASE + 0x02C)
#define GPIO_IRQENABLE_0   (GPIO1_BASE + 0x034)
#define GPIO_FALLINGDETECT (GPIO1_BASE + 0x14C)
#define GPIO_RISINGDETECT  (GPIO1_BASE + 0x148)
#define GPIO_DEBOUNCE_EN   (GPIO1_BASE + 0x150)
#define GPIO_LEVELDETECT0  (GPIO1_BASE + 0x140)
#define GPIO_LEVELDETECT1  (GPIO1_BASE + 0x144)
#define GPIO_IRQWAKEN_0    (GPIO1_BASE + 0x044)

/* -------- GPIO bits -------- */
#define GPIO1_12           (1u << 12)  /* Button - P8.12 */
#define GPIO1_13           (1u << 13)  /* LED - P8.11 */

/* -------- Pinmux -------- */
#define CONF_P8_12         (CONTROL_BASE + 0x030)  /* GPIO1_12 - Button */
#define CONF_P8_11         (CONTROL_BASE + 0x034)  /* GPIO1_13 - LED */

/* -------- Clock -------- */
#define CM_PER_GPIO1_CLKCTRL  (CM_PER_BASE + 0xAC)
#define CM_PER_TIMER2_CLKCTRL (CM_PER_BASE + 0x80)

/* -------- AINTC -------- */
#define INTC_SYSCONFIG     (AINTC_BASE + 0x10)
#define INTC_SIR_IRQ       (AINTC_BASE + 0x40)
#define INTC_CONTROL       (AINTC_BASE + 0x48)
#define INTC_MIR2          (AINTC_BASE + 0xA4)  /* IRQ 64-95 */
#define INTC_MIR_CLEAR2    (AINTC_BASE + 0xA8)
#define INTC_MIR_SET2      (AINTC_BASE + 0xAC)
#define INTC_MIR3          (AINTC_BASE + 0xC4)  /* IRQ 96-127 */
#define INTC_MIR_CLEAR3    (AINTC_BASE + 0xC8)
#define INTC_MIR_SET3      (AINTC_BASE + 0xCC)
#define INTC_PROTECTION    (AINTC_BASE + 0x4C)
#define INTC_IDLE          (AINTC_BASE + 0x50)

/* -------- GPTimer2 -------- */
#define GPT2_BASE      0x48040000
#define TISR          (GPT2_BASE + 0x28)
#define TIER          (GPT2_BASE + 0x2C)
#define TCLR          (GPT2_BASE + 0x38)
#define TCRR          (GPT2_BASE + 0x3C)
#define TLDR          (GPT2_BASE + 0x40)

/* IRQ numbers */
#define GPIO1_IRQ      99
#define GPTIMER2_IRQ   68

/* Watchdog */
#define WDT_BASE   0x44E35000
#define WDT_WSPR   (*(volatile unsigned int *)(WDT_BASE + 0x48))
#define WDT_WWPS   (*(volatile unsigned int *)(WDT_BASE + 0x34))

static volatile int led_state = 0;
volatile uint32_t timer_ticks = 0;

/* ========================= Debug helpers ========================= */
static void print_hex(uint32_t val)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4)
        uart_putc(hex_chars[(val >> i) & 0xF]);
}

static void print_dec_u32(uint32_t v)
{
    /* simple decimal printing without stdlib */
    char buf[11];
    int i = 0;
    if (v == 0) { uart_putc('0'); return; }
    while (v > 0 && i < 10) {
        buf[i++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (i--) uart_putc(buf[i]);
}

static void dump_gpio_registers(void)
{
    uart_puts("[DUMP] === GPIO1 Registers ===\n");
    uart_puts("[DUMP] OE=");            print_hex(REG32(GPIO_OE));            uart_putc('\n');
    uart_puts("[DUMP] DATAIN=");        print_hex(REG32(GPIO_DATAIN));        uart_putc('\n');
    uart_puts("[DUMP] IRQSTATUS_0=");   print_hex(REG32(GPIO_IRQSTATUS_0));   uart_putc('\n');
    uart_puts("[DUMP] IRQENABLE_0=");   print_hex(REG32(GPIO_IRQENABLE_0));   uart_putc('\n');
    uart_puts("[DUMP] FALLING=");       print_hex(REG32(GPIO_FALLINGDETECT)); uart_putc('\n');
    uart_puts("[DUMP] RISING=");        print_hex(REG32(GPIO_RISINGDETECT));  uart_putc('\n');
    uart_puts("[DUMP] LEVEL0=");        print_hex(REG32(GPIO_LEVELDETECT0));  uart_putc('\n');
    uart_puts("[DUMP] LEVEL1=");        print_hex(REG32(GPIO_LEVELDETECT1));  uart_putc('\n');
    uart_puts("[DUMP] DEBOUNCE_EN=");   print_hex(REG32(GPIO_DEBOUNCE_EN));   uart_putc('\n');
    uart_puts("[DUMP] IRQWAKEN_0=");    print_hex(REG32(GPIO_IRQWAKEN_0));    uart_putc('\n');
    uart_puts("[DUMP] ======================\n");
}

static void dump_aintc_registers(void)
{
    uart_puts("[DUMP] === AINTC Registers ===\n");
    uart_puts("[DUMP] CONTROL="); print_hex(REG32(INTC_CONTROL)); uart_putc('\n');
    uart_puts("[DUMP] MIR2=");    print_hex(REG32(INTC_MIR2));    uart_putc('\n');
    uart_puts("[DUMP] MIR3=");    print_hex(REG32(INTC_MIR3));    uart_putc('\n');
    uart_puts("[DUMP] SIR_IRQ="); print_hex(REG32(INTC_SIR_IRQ)); uart_putc('\n');
    uart_puts("[DUMP] ========================\n");
}

static void log_core_state(void)
{
    uint32_t cpsr, vbar;
    asm volatile ("mrs %0, cpsr" : "=r"(cpsr));
    asm volatile ("mrc p15, 0, %0, c12, c0, 0" : "=r"(vbar));

    uart_puts("[CORE] CPSR="); print_hex(cpsr);
    uart_puts(" IRQ="); uart_puts((cpsr & 0x80) ? "MASKED" : "ENABLED");
    uart_puts(" VBAR="); print_hex(vbar);
    uart_putc('\n');
}

/* ========================= ISRs ========================= */
void gpio1_isr(void)
{
    uart_puts("[GPIO] ISR entered\n");

    /* Clear pending for button (W1C) */
    REG32(GPIO_IRQSTATUS_0) = GPIO1_12;

    led_state ^= 1;
    if (led_state) {
        REG32(GPIO_SETDATAOUT) = GPIO1_13;
        uart_puts("[GPIO] LED ON\n");
    } else {
        REG32(GPIO_CLEARDATAOUT) = GPIO1_13;
        uart_puts("[GPIO] LED OFF\n");
    }
}

void gptimer2_isr(void)
{
    REG32(TISR) = 0x2;  /* clear overflow */
    timer_ticks++;
    uart_puts("[TIMER] tick #");
    print_dec_u32(timer_ticks);
    uart_putc('\n');
}

/* ========================= IRQ Dispatch ========================= */
void irq_dispatch(void)
{
    uint32_t irq = REG32(INTC_SIR_IRQ) & 0x7F;

    uart_puts("[IRQ] received: ");
    print_dec_u32(irq);
    uart_putc('\n');

    if (irq == GPIO1_IRQ) {
        uart_puts("[IRQ] source: GPIO1\n");
        gpio1_isr();
    } else if (irq == GPTIMER2_IRQ) {
        uart_puts("[IRQ] source: GPTIMER2\n");
        gptimer2_isr();
    } else {
        uart_puts("[IRQ] source: UNKNOWN irq=");
        print_dec_u32(irq);
        uart_putc('\n');
    }

    /* Acknowledge (new IRQ agreement) */
    REG32(INTC_CONTROL) = 1;
}

/* ========================= Init ========================= */
static void wdt_disable(void)
{
    WDT_WSPR = 0xAAAA;
    while (WDT_WWPS & (1 << 4));
    WDT_WSPR = 0x5555;
    while (WDT_WWPS & (1 << 4));
}

static void gpio1_clock_enable(void)
{
    REG32(CM_PER_GPIO1_CLKCTRL) = 0x2;
    while ((REG32(CM_PER_GPIO1_CLKCTRL) >> 16) & 3);
}

static void gptimer2_clock_enable(void)
{
    REG32(CM_PER_TIMER2_CLKCTRL) = 0x2;
    while ((REG32(CM_PER_TIMER2_CLKCTRL) >> 16) & 3);
}

static void pinmux_setup(void)
{
    /* Keep your values, but now we teach them:
       0x3F for input + pullup + mode 7 (GPIO), and 0x07 for GPIO output */
    REG32(CONF_P8_12) = 0x3F; /* Button: GPIO + input + pullup */
    REG32(CONF_P8_11) = 0x07; /* LED: GPIO output */
}

static void gpio_setup(void)
{
    uart_puts("[GPIO] Starting setup\n");

    /* Direction */
    REG32(GPIO_OE) |= GPIO1_12;    /* input */
    REG32(GPIO_OE) &= ~GPIO1_13;   /* output */
    uart_puts("[GPIO] OE configured\n");

    /* Debounce enable (optional; time register not set in this demo) */
    REG32(GPIO_DEBOUNCE_EN) |= GPIO1_12;
    uart_puts("[GPIO] Debounce enabled (EN only)\n");

    /* Edge detect: falling on button */
    REG32(GPIO_FALLINGDETECT) |= GPIO1_12;
    REG32(GPIO_RISINGDETECT)  &= ~GPIO1_12;
    REG32(GPIO_LEVELDETECT0)  &= ~GPIO1_12;
    REG32(GPIO_LEVELDETECT1)  &= ~GPIO1_12;
    uart_puts("[GPIO] Edge detect configured (falling)\n");

    /* Clear pending first (W1C) */
    uint32_t before = REG32(GPIO_IRQSTATUS_0);
    REG32(GPIO_IRQSTATUS_0) = GPIO1_12;
    uart_puts("[GPIO] Clear pending: before=");
    print_hex(before);
    uart_putc('\n');

    /* Enable interrupt generation on line 0 */
    REG32(GPIO_IRQENABLE_0) |= GPIO1_12;
    uart_puts("[GPIO] IRQENABLE_0 set for GPIO1_12\n");

#if TEACHING_VERBOSE_DUMPS
    dump_gpio_registers();
#endif

    /* Button connection snapshot */
    uart_puts("[GPIO] Button state now: ");
    uart_puts((REG32(GPIO_DATAIN) & GPIO1_12) ? "HIGH (released)\n" : "LOW (pressed)\n");
}

static void gptimer2_init(void)
{
    REG32(TLDR) = 0xFFFF0000;
    REG32(TCRR) = 0xFFFF0000;
    REG32(TIER) = 0x2;  /* overflow interrupt */
    REG32(TCLR) = 0x3;  /* start timer */
    uart_puts("[TIMER] GPTimer2 started (overflow IRQ)\n");
}

static void aintc_setup(void)
{
    uart_puts("[AINTC] Starting setup\n");

    REG32(INTC_SYSCONFIG)  = 0x2;  /* soft reset */
    REG32(INTC_PROTECTION) = 0x0;  /* disable protection */
    REG32(INTC_IDLE)       = 0x0;  /* disable auto-gating */

    /* Unmask GPIO1 IRQ=99 -> MIR3 bit(99-96)=3 */
    uint32_t gpio_bit  = (GPIO1_IRQ - 96);
    /* Unmask Timer2 IRQ=68 -> MIR2 bit(68-64)=4 */
    uint32_t tim_bit   = (GPTIMER2_IRQ - 64);

    /* (Optional) mask then unmask (teaching) */
    REG32(INTC_MIR_SET3)   = (1u << gpio_bit);
    REG32(INTC_MIR_SET2)   = (1u << tim_bit);

    REG32(INTC_MIR_CLEAR3) = (1u << gpio_bit);
    REG32(INTC_MIR_CLEAR2) = (1u << tim_bit);

    uart_puts("[AINTC] Unmasked GPIO1 IRQ99 at MIR3 bit ");
    print_dec_u32(gpio_bit);
    uart_putc('\n');

    uart_puts("[AINTC] Unmasked GPTimer2 IRQ68 at MIR2 bit ");
    print_dec_u32(tim_bit);
    uart_putc('\n');

#if TEACHING_VERBOSE_DUMPS
    dump_aintc_registers();
#endif
}

static void set_vector_base(void)
{
    extern uint32_t _start;
    uint32_t vector_addr = (uint32_t)&_start;

    uart_puts("[VBAR] Setting VBAR=");
    print_hex(vector_addr);
    uart_putc('\n');

    asm volatile ("mcr p15, 0, %0, c12, c0, 0" : : "r"(vector_addr));

    uint32_t vbar_read;
    asm volatile ("mrc p15, 0, %0, c12, c0, 0" : "=r"(vbar_read));

    uart_puts("[VBAR] Readback VBAR=");
    print_hex(vbar_read);
    uart_putc('\n');
}

static inline void enable_irq(void)
{
    asm volatile ("cpsie i");
}

/* ========================= main ========================= */
int main(void)
{
    wdt_disable();
    uart_init();
    uart_puts("[BOOT] main entered\n");

    set_vector_base();
    uart_puts("[BOOT] vector base set\n");

    gpio1_clock_enable();
    uart_puts("[BOOT] GPIO1 clock enabled\n");

    gptimer2_clock_enable();
    uart_puts("[BOOT] GPTimer2 clock enabled\n");

    pinmux_setup();
    uart_puts("[BOOT] pinmux configured\n");

    gpio_setup();
    uart_puts("[BOOT] GPIO configured\n");

    gptimer2_init();

    aintc_setup();
    uart_puts("[BOOT] AINTC configured\n");

    log_core_state();

    enable_irq();
    uart_puts("[BOOT] IRQ enabled globally (cpsie i)\n");
    log_core_state();

    uart_puts("[INFO] Press P8.12 button => expect IRQ99 and LED toggle\n");

    uint32_t loop_count = 0;
    while (1) {
        loop_count++;

#if TEACHING_POLL_ASSIST
        /* Teaching assist only: show pending flag if it ever sets */
        uint32_t st = REG32(GPIO_IRQSTATUS_0);
        if (st & GPIO1_12) {
            uart_puts("[POLL] Pending IRQSTATUS_0 for GPIO1_12 detected (should be handled by IRQ)\n");
            uart_puts("[POLL] IRQSTATUS_0="); print_hex(st); uart_putc('\n');
            /* clear it so we don't get stuck */
            REG32(GPIO_IRQSTATUS_0) = GPIO1_12;
        }
#endif

        asm volatile ("wfi");
    }
}
