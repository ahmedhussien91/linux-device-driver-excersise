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
#define GPIO_SYSCONFIG     (GPIO1_BASE + 0x10)
#define GPIO_SYSSTATUS     (GPIO1_BASE + 0x114)
#define GPIO_CTRL          (GPIO1_BASE + 0x130)  /* global control */
#define GPIO_OE            (GPIO1_BASE + 0x134)
#define GPIO_DATAIN        (GPIO1_BASE + 0x138)
#define GPIO_SETDATAOUT    (GPIO1_BASE + 0x194)
#define GPIO_CLEARDATAOUT  (GPIO1_BASE + 0x190)
#define GPIO_IRQSTATUS_0   (GPIO1_BASE + 0x02C)
#define GPIO_IRQENABLE_0   (GPIO1_BASE + 0x034)
#define GPIO_IRQSTATUS_1   (GPIO1_BASE + 0x030)
#define GPIO_IRQENABLE_1   (GPIO1_BASE + 0x038)
#define GPIO_FALLINGDETECT (GPIO1_BASE + 0x14C)
#define GPIO_RISINGDETECT  (GPIO1_BASE + 0x148)
#define GPIO_DEBOUNCE_EN   (GPIO1_BASE + 0x150)
#define GPIO_LEVELDETECT0  (GPIO1_BASE + 0x140)
#define GPIO_LEVELDETECT1  (GPIO1_BASE + 0x144)
#define GPIO_IRQWAKEN_0    (GPIO1_BASE + 0x044)

/* -------- GPIO bits -------- */
#define GPIO1_12           (1u << 12)  /* LED - P8.12 */
#define GPIO1_15           (1u << 15)  /* Button - P8.15 */

/* -------- Pinmux -------- */
#define CONF_P8_12         (CONTROL_BASE + 0x830)  /* GPIO1_12 - LED */
#define CONF_P8_15         (CONTROL_BASE + 0x83C)  /* GPIO1_15 - Button */

/* -------- Clock -------- */
#define CM_PER_GPIO1_CLKCTRL  (CM_PER_BASE + 0xAC)
#define CM_PER_TIMER2_CLKCTRL (CM_PER_BASE + 0x80)

/* -------- AINTC -------- */
#define INTC_SYSSTATUS      (AINTC_BASE + 0x14)
#define INTC_ILR(n)         (AINTC_BASE + 0x100 + 4U*(n))  /* Interrupt Line Register */
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
#define INTC_PENDING_IRQ2   (AINTC_BASE + 0xD8)  /* IRQ 64-95 pending */
#define INTC_PENDING_IRQ3   (AINTC_BASE + 0xF8)  /* IRQ 96-127 pending */

#define CM_PER_L4LS_CLKSTCTRL (CM_PER_BASE + 0x00)   /* L4LS clock state control */

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
    uart_puts("[DUMP] IRQSTATUS_1=");   print_hex(REG32(GPIO_IRQSTATUS_1));   uart_putc('\n');
    uart_puts("[DUMP] IRQENABLE_1=");   print_hex(REG32(GPIO_IRQENABLE_1));   uart_putc('\n');
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

static void debug_interrupt_setup(void)
{
    uint32_t cpsr, vbar;
    asm volatile ("mrs %0, cpsr" : "=r"(cpsr));
    asm volatile ("mrc p15, 0, %0, c12, c0, 0" : "=r"(vbar));

    uart_puts("[DEBUG] === Interrupt Setup Check ===\n");
    uart_puts("[DEBUG] CPSR="); print_hex(cpsr);
    uart_puts(" Mode="); print_hex(cpsr & 0x1F);
    uart_puts(" IRQ="); uart_puts((cpsr & 0x80) ? "MASKED" : "ENABLED");
    uart_putc('\n');

    uart_puts("[DEBUG] VBAR="); print_hex(vbar); uart_putc('\n');

    /* Check vector table content */
    uint32_t *vectors = (uint32_t *)vbar;
    uart_puts("[DEBUG] Vector[0]="); print_hex(vectors[0]); uart_putc('\n');
    uart_puts("[DEBUG] Vector[6]="); print_hex(vectors[6]); uart_puts(" (IRQ)\n");

    uart_puts("[DEBUG] GPIO_IRQENABLE_0="); print_hex(REG32(GPIO_IRQENABLE_0)); uart_putc('\n');
    uart_puts("[DEBUG] GPIO_IRQSTATUS_0="); print_hex(REG32(GPIO_IRQSTATUS_0)); uart_putc('\n');
    uart_puts("[DEBUG] INTC_MIR3="); print_hex(REG32(INTC_MIR3)); uart_putc('\n');
    uart_puts("[DEBUG] INTC_SIR_IRQ="); print_hex(REG32(INTC_SIR_IRQ)); uart_putc('\n');
    uart_puts("[DEBUG] ==============================\n");
}

/* ========================= ISRs ========================= */
void gpio1_isr(void)
{
    uart_puts("[GPIO] ISR entered\n");

    /* Clear pending for button (W1C) */
    REG32(GPIO_IRQSTATUS_0) = GPIO1_15;
    REG32(GPIO_IRQSTATUS_1) = GPIO1_15;

    /* Read current button state to determine press/release */
    uint32_t button_state = REG32(GPIO_DATAIN) & GPIO1_15;

    if (button_state == 0) {
        /* Button is LOW (pressed) - turn LED OFF */
        REG32(GPIO_CLEARDATAOUT) = GPIO1_12;
        led_state = 0;
        uart_puts("[GPIO] Button PRESSED - LED OFF\n");
    } else {
        /* Button is HIGH (released) - turn LED ON */
        REG32(GPIO_SETDATAOUT) = GPIO1_12;
        led_state = 1;
        uart_puts("[GPIO] Button RELEASED - LED ON\n");
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
    REG32(INTC_CONTROL) = 1;
    uint32_t irq = REG32(INTC_SIR_IRQ) & 0x7F;

    uart_puts("[IRQ] received: ");
    print_dec_u32(irq);
    uart_putc('\n');

    if (irq == GPIO1_IRQ) {
        uart_puts("[IRQ] source: GPIO1\n");
        gpio1_isr();
    }
    else if (irq == GPTIMER2_IRQ) {
        uart_puts("[IRQ] source: GPTIMER2\n");
        gptimer2_isr();
    }
    else {
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
    uart_puts("[CM] Enabling GPIO1 clock...\n");
    REG32(CM_PER_GPIO1_CLKCTRL) = 0x2; /* MODULEMODE=ENABLE */

    /* Wait for IDLEST=0 (functional) */
    while (((REG32(CM_PER_GPIO1_CLKCTRL) >> 16) & 0x3) != 0x0) { }

    uart_puts("[CM] GPIO1_CLKCTRL=");
    print_hex(REG32(CM_PER_GPIO1_CLKCTRL));
    uart_putc('\n');
}


static void gpio1_module_init(void)
{
    uart_puts("[GPIO] Module init\n");

    uart_puts("[GPIO] SYSCONFIG(before)=");
    print_hex(REG32(GPIO_SYSCONFIG));
    uart_puts(" SYSSTATUS(before)=");
    print_hex(REG32(GPIO_SYSSTATUS));
    uart_putc('\n');

    /* طلب softreset */
    REG32(GPIO_SYSCONFIG) |= (1U << 1);  /* SOFTRESET = bit1 */
    uart_puts("[GPIO] SOFTRESET requested\n");

    /* بدل ما نستنى SYSSTATUS (ممكن ما يشتغلش), نستنى SOFTRESET يصفّر نفسه */
    uint32_t timeout = 5000000;
    while ((REG32(GPIO_SYSCONFIG) & (1U << 1)) && timeout--) {
        /* spin */
    }

    uart_puts("[GPIO] SYSCONFIG(after wait)=");
    print_hex(REG32(GPIO_SYSCONFIG));
    uart_puts(" SYSSTATUS(after wait)=");
    print_hex(REG32(GPIO_SYSSTATUS));
    uart_putc('\n');

    if (timeout == 0) {
        uart_puts("[GPIO] WARNING: SOFTRESET wait timed out (continuing)\n");
    }
    else {
        uart_puts("[GPIO] Reset completed (SOFTRESET cleared)\n");
    }

    /* تأكد أن GPIO module enabled (DISABLEMODULE=0) */
    REG32(GPIO_CTRL) = 0x0;
    uart_puts("[GPIO] GPIO_CTRL set: module enabled\n");

    uart_puts("[GPIO] CTRL=");
    print_hex(REG32(GPIO_CTRL));
    uart_putc('\n');
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
    REG32(CONF_P8_12) = 0x07; /* LED: GPIO output */
    REG32(CONF_P8_15) = 0x3F; /* Button: GPIO + input + pullup */
}

static void gpio_setup(void)
{
    uart_puts("[GPIO] Starting setup\n");

    /* Direction */
    REG32(GPIO_OE) |= GPIO1_15;    /* input */
    REG32(GPIO_OE) &= ~GPIO1_12;   /* output */
    uart_puts("[GPIO] OE configured\n");

    /* Debounce enable (optional; time register not set in this demo) */
//     REG32(GPIO_DEBOUNCE_EN) |= GPIO1_15;
//     uart_puts("[GPIO] Debounce enabled (EN only)\n");
    REG32(GPIO_DEBOUNCE_EN) &= ~GPIO1_15;
    uart_puts("[GPIO] Debounce DISABLED for test\n");

    /* Edge detect: both falling and rising on button */
    REG32(GPIO_FALLINGDETECT) |= GPIO1_15;  /* button press */
    REG32(GPIO_RISINGDETECT) |= GPIO1_15;   /* button release */
    REG32(GPIO_LEVELDETECT0) &= ~GPIO1_15;
    REG32(GPIO_LEVELDETECT1) &= ~GPIO1_15;
    uart_puts("[GPIO] Edge detect configured (falling + rising)\n");

    /* Clear pending first (W1C) */
    uint32_t before = REG32(GPIO_IRQSTATUS_0);
    REG32(GPIO_IRQSTATUS_0) = GPIO1_15;
    uart_puts("[GPIO] Clear pending: before=");
    print_hex(before);
    uart_putc('\n');
    before = REG32(GPIO_IRQSTATUS_1);
    REG32(GPIO_IRQSTATUS_1) = GPIO1_15;
    uart_puts("[GPIO] Clear pending: before=");
    print_hex(before);
    uart_putc('\n');

    /* Enable interrupt generation on line 0 */
    REG32(GPIO_IRQENABLE_0) |= GPIO1_15;
    REG32(GPIO_IRQENABLE_1) |= GPIO1_15;
    uart_puts("[GPIO] IRQENABLE_0 set for GPIO1_15\n");

#if TEACHING_VERBOSE_DUMPS
    dump_gpio_registers();
#endif

    /* Button connection snapshot */
    uart_puts("[GPIO] Button state now: ");
    uart_puts((REG32(GPIO_DATAIN) & GPIO1_15) ? "HIGH (released)\n" : "LOW (pressed)\n");
}

static void cm_l4ls_wakeup(void)
{
    /* Force wakeup for L4LS domain (needed sometimes for interrupt propagation) */
    REG32(CM_PER_L4LS_CLKSTCTRL) = 0x2;  /* SW_WKUP */
    uart_puts("[CM] L4LS_CLKSTCTRL=");
    print_hex(REG32(CM_PER_L4LS_CLKSTCTRL));
    uart_putc('\n');
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

    REG32(INTC_SYSCONFIG) = 0x2;  /* soft reset */
    /* Wait reset done (THIS IS IMPORTANT) */
    while ((REG32(INTC_SYSSTATUS) & 0x1) == 0) {
        /* spin */
    }
    uart_puts("[AINTC] Reset done\n");
    REG32(INTC_PROTECTION) = 0x0;  /* disable protection */
    REG32(INTC_IDLE) = 0x0;  /* disable auto-gating */

    /* CRITICAL: Set NEWIRQAGR to enable new IRQ generation */
    REG32(INTC_CONTROL) = 0x1;
    uart_puts("[AINTC] Enabled new IRQ agreement\n");

    /* Route selected interrupts to IRQ (not FIQ), priority = 0 */
    REG32(INTC_ILR(98)) = 0x0;  /* GPIO1A -> IRQ */
    REG32(INTC_ILR(99)) = 0x0;  /* GPIO1B -> IRQ */
    REG32(INTC_ILR(68)) = 0x0;  /* GPTimer2 -> IRQ */
    uart_puts("[AINTC] ILR routing set: GPIO1A/GPIO1B/TIMER2 -> IRQ\n");

    /* Unmask GPIO1 IRQ=99 -> MIR3 bit(99-96)=3 */
    uint32_t gpio_bit = (GPIO1_IRQ - 96);
    /* Unmask Timer2 IRQ=68 -> MIR2 bit(68-64)=4 */
    uint32_t tim_bit = (GPTIMER2_IRQ - 64);

    /* CRITICAL FIX: After reset, ALL interrupts are masked (0xFFFFFFFF)
     * We need to unmask specific interrupts */
    uart_puts("[AINTC] Before unmask - MIR3="); print_hex(REG32(INTC_MIR3)); uart_putc('\n');
    uart_puts("[AINTC] CRITICAL: MIR3 should be 0xFFFFFFFF (all masked)\n");

    /* Clear specific interrupt masks */
    REG32(INTC_MIR_CLEAR3) = (1u << gpio_bit);  /* Unmask GPIO1 IRQ99 */
    REG32(INTC_MIR_CLEAR2) = (1u << tim_bit);   /* Unmask Timer2 IRQ68 */

    uart_puts("[AINTC] After unmask - MIR3="); print_hex(REG32(INTC_MIR3)); uart_putc('\n');
    uart_puts("[AINTC] CRITICAL: MIR3 should now have bit "); print_dec_u32(gpio_bit); uart_puts(" cleared\n");
    dump_aintc_registers();
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

static inline void setup_arm_irq_mode(void)
{
    uint32_t cpsr_before, cpsr_after;

    uart_puts("[ARM] Before mode change...\n");
    asm volatile ("mrs %0, cpsr" : "=r"(cpsr_before));
    uart_puts("[ARM] Current CPSR="); print_hex(cpsr_before); uart_putc('\n');

    /* Just enable IRQs first, don't change mode yet */
    asm volatile (
        "mrs r0, cpsr\n"
        "bic r0, r0, #0x80\n"     /* Clear IRQ disable bit only */
        "msr cpsr_c, r0\n"
        : : : "r0"
    );

    asm volatile ("mrs %0, cpsr" : "=r"(cpsr_after));
    uart_puts("[ARM] After IRQ enable - CPSR="); print_hex(cpsr_after); uart_putc('\n');
    uart_puts("[ARM] IRQ enable complete\n");
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
    cm_l4ls_wakeup();
    uart_puts("[BOOT] GPIO1 clock enabled\n");

    gpio1_module_init();
    uart_puts("[GPIO] After module init: OE=");
    print_hex(REG32(GPIO_OE));
    uart_puts(" DATAIN=");
    print_hex(REG32(GPIO_DATAIN));
    uart_putc('\n');


    gptimer2_clock_enable();
    uart_puts("[BOOT] GPTimer2 clock enabled\n");

    pinmux_setup();
    uart_puts("[BOOT] pinmux configured\n");
    uart_puts("[PINMUX] CONF_P8_12="); print_hex(REG32(CONF_P8_12)); uart_putc('\n');
    uart_puts("[PINMUX] CONF_P8_15="); print_hex(REG32(CONF_P8_15)); uart_putc('\n');

    gpio_setup();
    uart_puts("[BOOT] GPIO configured\n");
    uart_puts("[TEST] DATAIN=");
    print_hex(REG32(GPIO_DATAIN));
    uart_puts(" GPIO1_15=");
    uart_puts((REG32(GPIO_DATAIN) & GPIO1_15) ? "1 (HIGH)\n" : "0 (LOW)\n");

    gptimer2_init();

    uart_puts("[MANUAL TEST] Press and hold button, then type something...\n");
    for (int i = 0; i < 5; i++) {
        uint32_t datain = REG32(GPIO_DATAIN);
        uint32_t btn_state = datain & GPIO1_15;
        uart_puts("[MANUAL] DATAIN="); print_hex(datain);
        uart_puts(" BTN="); uart_puts(btn_state ? "HIGH" : "LOW");
        uart_puts(" IRQ_STATUS_0="); print_hex(REG32(GPIO_IRQSTATUS_0));
        uart_putc('\n');

        /* Delay */
        for (volatile int d = 0; d < 1000000; d++);
    }

    debug_interrupt_setup();

    uart_puts("[TEST1] Waiting for GPIO_IRQSTATUS_0 (with timeout)...\n");
    uint32_t timeout_counter = 0;
    while (1) {
        uint32_t s0 = REG32(GPIO_IRQSTATUS_0);
        uint32_t s1 = REG32(GPIO_IRQSTATUS_1);
        if ((s0 | s1) & GPIO1_15) {
            uart_puts("[TEST1] GPIO IRQ latched. NOT clearing yet.\n");
            uart_puts("[TEST1] IRQSTATUS_0="); print_hex(s0);
            uart_puts(" IRQSTATUS_1="); print_hex(s1);
            uart_putc('\n');
            break;
        }

        timeout_counter++;
        if (timeout_counter > 10000000) {
            uart_puts("[TEST1] TIMEOUT! No GPIO interrupt detected.\n");
            uart_puts("[TEST1] Current DATAIN="); print_hex(REG32(GPIO_DATAIN));
            uart_puts(" Expected button bit="); print_hex(GPIO1_15);
            uart_putc('\n');
            uart_puts("[TEST1] Try pressing the button NOW...\n");
            timeout_counter = 0;
        }
    }

    uart_puts("[PROOF] Now polling AINTC while GPIO status is still set...\n");
    for (uint32_t i=0; i<5000000; i++) {
        uint32_t sir = REG32(INTC_SIR_IRQ);
        if ((sir & 0x7F) != 0x80) {
            uart_puts("[PROOF] SIR_IRQ=");
            print_hex(sir);
            uart_putc('\n');
            break;
        }
    }

    /* NOW clear it */
    REG32(GPIO_IRQSTATUS_0) = GPIO1_15;
    REG32(GPIO_IRQSTATUS_1) = GPIO1_15;
    uart_puts("[TEST1] Cleared GPIO IRQSTATUS\n");

    uart_puts("[PROOF] Press button now; polling SIR_IRQ...\n");
    while (1) {
        uint32_t sir = REG32(INTC_SIR_IRQ);
        if ((sir & 0x7F) != 0x80) {  /* not spurious */
            uart_puts("[PROOF] SIR_IRQ=");
            print_hex(sir);
            uart_putc('\n');
            break;
        }
    }

    aintc_setup();
    uart_puts("[BOOT] AINTC configured\n");

    debug_interrupt_setup();

    setup_arm_irq_mode();
    uart_puts("[BOOT] IRQ enabled and ARM mode set\n");
    debug_interrupt_setup();

    uart_puts("[INFO] Press P8.15 button => LED OFF, Release => LED ON\n");

    uart_puts("[TIMER TEST] Waiting for TISR overflow...\n");
    while (1) {
        if (REG32(TISR) & 0x2) {
            uart_puts("[TIMER TEST] Overflow flag SET\n");
            REG32(TISR) = 0x2;
            break;
        }
    }

    /* Now do PROOF polling */
    uart_puts("[PROOF] Press button now; polling SIR_IRQ...\n");
    while (1) {
        uint32_t sir = REG32(INTC_SIR_IRQ);
        if ((sir & 0x7F) != 0x80) {
            uart_puts("[PROOF] SIR_IRQ=");
            print_hex(sir);
            uart_putc('\n');
            break;
        }
    }

    uint32_t loop_count = 0;
    while (1) {
        loop_count++;

#if TEACHING_POLL_ASSIST
        /* Teaching assist only: show pending flag if it ever sets */
        uint32_t st = REG32(GPIO_IRQSTATUS_0);
        if (st & GPIO1_15) {
            uart_puts("[POLL] Pending IRQSTATUS_0 for GPIO1_15 detected (should be handled by IRQ)\n");
            uart_puts("[DBG] IRQSTATUS_0="); print_hex(REG32(GPIO_IRQSTATUS_0));
            uart_puts(" IRQSTATUS_1=");      print_hex(REG32(GPIO_IRQSTATUS_1));
            uart_putc('\n');            /* clear it so we don't get stuck */
            REG32(GPIO_IRQSTATUS_0) = GPIO1_15;
            uart_puts("[AINTC] PENDING3="); print_hex(REG32(INTC_PENDING_IRQ3));
            uart_puts(" SIR_IRQ=");         print_hex(REG32(INTC_SIR_IRQ));
            uart_putc('\n');

        }
#endif

        asm volatile ("wfi");
    }
}
