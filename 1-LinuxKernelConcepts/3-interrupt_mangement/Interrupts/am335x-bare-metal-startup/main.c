/*
 * main.c - BeagleBone Black (AM335x) Bare-Metal Main Program
 * Target: ARM Cortex-A8 (ARMv7-A)
 * 
 * This program demonstrates:
 * - AINTC (ARM Interrupt Controller) initialization
 * - DMTimer interrupt setup and handling  
 * - Proper interrupt acknowledgment flow
 */

#include <stdint.h>

/*
 * AM335x Memory Map - Key Peripheral Base Addresses
 */

/* ARM Interrupt Controller (AINTC) */
#define AINTC_BASE                  0x48200000
#define AINTC_SYSCONFIG             (AINTC_BASE + 0x10)
#define AINTC_SYSSTATUS             (AINTC_BASE + 0x14)
#define AINTC_SIR_IRQ               (AINTC_BASE + 0x40)
#define AINTC_SIR_FIQ               (AINTC_BASE + 0x44)
#define AINTC_CONTROL               (AINTC_BASE + 0x48)
#define AINTC_PROTECTION            (AINTC_BASE + 0x4C)
#define AINTC_IDLE                  (AINTC_BASE + 0x50)
#define AINTC_IRQ_PRIORITY          (AINTC_BASE + 0x60)
#define AINTC_FIQ_PRIORITY          (AINTC_BASE + 0x64)
#define AINTC_THRESHOLD             (AINTC_BASE + 0x68)
#define AINTC_MIR_CLEAR(n)          (AINTC_BASE + 0x88 + (n * 0x20))
#define AINTC_MIR_SET(n)            (AINTC_BASE + 0x8C + (n * 0x20))
#define AINTC_ISR_SET(n)            (AINTC_BASE + 0x90 + (n * 0x20))
#define AINTC_ISR_CLEAR(n)          (AINTC_BASE + 0x94 + (n * 0x20))
#define AINTC_PENDING_IRQ(n)        (AINTC_BASE + 0x98 + (n * 0x20))
#define AINTC_PENDING_FIQ(n)        (AINTC_BASE + 0x9C + (n * 0x20))

/* DMTimer1 MS (1ms timer) */
#define DMTIMER1MS_BASE             0x44E31000
#define DMTIMER_TIDR                0x00
#define DMTIMER_TIOCP_CFG           0x10
#define DMTIMER_TISTAT              0x14
#define DMTIMER_TISR                0x18
#define DMTIMER_TIER                0x1C
#define DMTIMER_TWER                0x20
#define DMTIMER_TCLR                0x24
#define DMTIMER_TCRR                0x28
#define DMTIMER_TLDR                0x2C
#define DMTIMER_TTGR                0x30
#define DMTIMER_TWPS                0x34
#define DMTIMER_TMAR                0x38
#define DMTIMER_TCAR1               0x3C
#define DMTIMER_TSICR               0x40
#define DMTIMER_TCAR2               0x44

/* Interrupt numbers */
#define INT_DMTIMER1MS              67  /* DMTimer1MS interrupt */

/* Control Module for clock enable */
#define CM_WKUP_BASE                0x44E00400
#define CM_WKUP_TIMER1_CLKCTRL      (CM_WKUP_BASE + 0xC4)

/* Register access macros */
#define REG32(addr)                 (*((volatile uint32_t *)(addr)))

/*
 * Global variables
 */
static volatile uint32_t timer_count = 0;

/*
 * Function prototypes
 */
void init_aintc(void);
void init_dmtimer(void);
void enable_interrupt(uint32_t int_num);
void disable_interrupt(uint32_t int_num);
void c_irq_handler(void);
void delay_ms(uint32_t ms);

/*
 * Memory barrier functions
 */
static inline void memory_barrier(void)
{
    asm volatile ("dmb" : : : "memory");
}

static inline void instruction_sync_barrier(void)
{
    asm volatile ("isb" : : : "memory");
}

/*
 * Enable IRQs in CPSR
 */
static inline void enable_irq(void)
{
    asm volatile ("cpsie i" : : : "memory");
}

/*
 * Disable IRQs in CPSR  
 */
static inline void disable_irq(void)
{
    asm volatile ("cpsid i" : : : "memory");
}

/*
 * Main function - called from startup.s
 */
int main(void)
{
    /* Initialize AINTC interrupt controller */
    init_aintc();
    
    /* Initialize and start DMTimer */
    init_dmtimer();
    
    /* Enable DMTimer interrupt */
    enable_interrupt(INT_DMTIMER1MS);
    
    /* Enable IRQs globally */
    enable_irq();
    
    /* Main application loop */
    uint32_t last_count = 0;
    
    while (1) {
        /* Check if timer interrupt occurred */
        if (timer_count != last_count) {
            last_count = timer_count;
            
            /* Simple indication that we're running */
            /* In real hardware, this could toggle an LED */
            if ((timer_count % 1000) == 0) {
                /* Print every second (assuming 1ms timer) */
                /* Note: No UART in this minimal example */
            }
        }
        
        /* Wait for interrupt */
        asm volatile ("wfi");
    }
    
    return 0;
}

/*
 * Initialize ARM Interrupt Controller (AINTC)
 */
void init_aintc(void)
{
    /* Reset AINTC */
    REG32(AINTC_SYSCONFIG) = 0x02;  /* Soft reset */
    
    /* Wait for reset completion */
    while ((REG32(AINTC_SYSSTATUS) & 0x01) == 0);
    
    /* Configure AINTC */
    REG32(AINTC_SYSCONFIG) = 0x00;  /* No idle modes, no auto-idle */
    
    /* Disable protection (allows access to other registers) */
    REG32(AINTC_PROTECTION) = 0x00;
    
    /* Set priority threshold to 0xFF (lowest priority) */
    REG32(AINTC_THRESHOLD) = 0xFF;
    
    /* Mask all interrupts initially */
    REG32(AINTC_MIR_SET(0)) = 0xFFFFFFFF;
    REG32(AINTC_MIR_SET(1)) = 0xFFFFFFFF;
    REG32(AINTC_MIR_SET(2)) = 0xFFFFFFFF;
    
    /* Clear any pending interrupts */
    REG32(AINTC_ISR_CLEAR(0)) = 0xFFFFFFFF;
    REG32(AINTC_ISR_CLEAR(1)) = 0xFFFFFFFF;
    REG32(AINTC_ISR_CLEAR(2)) = 0xFFFFFFFF;
    
    /* Enable new IRQ/FIQ generation */
    REG32(AINTC_CONTROL) = 0x03;
    
    memory_barrier();
}

/*
 * Initialize DMTimer1MS for periodic interrupts
 */
void init_dmtimer(void)
{
    /* Enable DMTimer1 clock */
    REG32(CM_WKUP_TIMER1_CLKCTRL) = 0x02;  /* Enable module */
    
    /* Wait for clock to be enabled */
    while ((REG32(CM_WKUP_TIMER1_CLKCTRL) & 0x30000) != 0);
    
    /* Reset DMTimer */
    REG32(DMTIMER1MS_BASE + DMTIMER_TIOCP_CFG) = 0x02;
    
    /* Wait for reset completion */
    while ((REG32(DMTIMER1MS_BASE + DMTIMER_TISTAT) & 0x01) == 0);
    
    /* Configure timer for 1ms periodic interrupt */
    /* Assuming 24MHz clock, for 1ms: reload = 0xFFFFFFFF - 24000 + 1 */
    uint32_t reload_value = 0xFFFFFFFF - 24000 + 1;
    
    /* Set reload value */
    REG32(DMTIMER1MS_BASE + DMTIMER_TLDR) = reload_value;
    REG32(DMTIMER1MS_BASE + DMTIMER_TCRR) = reload_value;
    
    /* Enable overflow interrupt */
    REG32(DMTIMER1MS_BASE + DMTIMER_TIER) = 0x02;
    
    /* Configure timer control:
     * - Auto reload
     * - Start timer
     */
    REG32(DMTIMER1MS_BASE + DMTIMER_TCLR) = 0x03;
    
    memory_barrier();
}

/*
 * Enable specific interrupt in AINTC
 */
void enable_interrupt(uint32_t int_num)
{
    uint32_t reg_offset = int_num / 32;
    uint32_t bit_offset = int_num % 32;
    
    /* Clear mask bit to enable interrupt */
    REG32(AINTC_MIR_CLEAR(reg_offset)) = (1 << bit_offset);
    
    memory_barrier();
}

/*
 * Disable specific interrupt in AINTC
 */
void disable_interrupt(uint32_t int_num)
{
    uint32_t reg_offset = int_num / 32;
    uint32_t bit_offset = int_num % 32;
    
    /* Set mask bit to disable interrupt */
    REG32(AINTC_MIR_SET(reg_offset)) = (1 << bit_offset);
    
    memory_barrier();
}

/*
 * C IRQ Handler - called from assembly IRQ handler
 * This function implements the complete interrupt handling flow:
 * 1. Read interrupt ID from AINTC
 * 2. Handle the specific interrupt
 * 3. Clear peripheral interrupt 
 * 4. Acknowledge AINTC
 */
void c_irq_handler(void)
{
    /* Read active IRQ number from AINTC */
    uint32_t sir_irq = REG32(AINTC_SIR_IRQ);
    uint32_t active_irq = sir_irq & 0x7F;  /* Bits 6:0 contain IRQ number */
    
    /* Check if this is a spurious interrupt */
    if (active_irq == 0x7F) {
        /* Spurious interrupt, just acknowledge and return */
        REG32(AINTC_CONTROL) = 0x01;  /* New IRQ acknowledge */
        return;
    }
    
    /* Handle specific interrupts */
    switch (active_irq) {
        case INT_DMTIMER1MS:
            /* DMTimer1MS overflow interrupt */
            
            /* Clear timer interrupt flag */
            REG32(DMTIMER1MS_BASE + DMTIMER_TISR) = 0x02;
            
            /* Increment our timer counter */
            timer_count++;
            
            break;
            
        default:
            /* Unknown interrupt - just clear and acknowledge */
            break;
    }
    
    /* Acknowledge interrupt to AINTC to allow new IRQs */
    REG32(AINTC_CONTROL) = 0x01;  /* New IRQ acknowledge */
    
    memory_barrier();
}

/*
 * Simple delay function using timer count
 * Note: This requires timer interrupts to be working
 */
void delay_ms(uint32_t ms)
{
    uint32_t start_count = timer_count;
    
    while ((timer_count - start_count) < ms) {
        asm volatile ("wfi");
    }
}