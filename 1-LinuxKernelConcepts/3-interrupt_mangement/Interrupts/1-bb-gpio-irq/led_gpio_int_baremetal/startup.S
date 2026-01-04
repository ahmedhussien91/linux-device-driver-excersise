.syntax unified
.cpu cortex-a8
.arm

.global _start
.global irq_handler

/* =========================================================
 * Vector Table
 * ========================================================= */
.section .vectors, "ax"
.align 5                  /* 32-byte alignment (MANDATORY) */

_start:
    b reset               /* 0x00 Reset */
    b undefined_handler   /* 0x04 Undefined */
    b swi_handler         /* 0x08 SWI */
    b prefetch_abort      /* 0x0C Prefetch Abort */
    b data_abort          /* 0x10 Data Abort */
    b .                   /* 0x14 Reserved */
    b irq_handler         /* 0x18 IRQ */
    b fiq_handler         /* 0x1C FIQ */

/* =========================================================
 * Reset handler
 * ========================================================= */
.section .text
reset:
    cpsid if              /* Disable IRQ & FIQ */

    /* Set stack pointer */
    ldr sp, =__stack_top__

    /* Jump to C main */
    bl main

1:  b 1b                  /* main should never return */

/* =========================================================
 * IRQ handler (low-level)
 * ========================================================= */
irq_handler:
    sub lr, lr, #4        /* Fix LR for IRQ mode */

    /* Save minimal context (ABI-safe) */
    stmfd sp!, {r0-r3, r12, lr}

    /* Call C dispatcher (prints happen there) */
    bl irq_dispatch

    /* Restore context */
    ldmfd sp!, {r0-r3, r12, lr}

    subs pc, lr, #0       /* Return from IRQ */

/* =========================================================
 * Dummy handlers (for teaching & safety)
 * ========================================================= */
undefined_handler:
swi_handler:
prefetch_abort:
data_abort:
fiq_handler:
    b .

/* =========================================================
 * Stack
 * ========================================================= */
.section .bss
.align 8
.space 4096
_stack_top:
.section .note.GNU-stack,"",%progbits
