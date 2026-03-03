/*
 * startup.s - BeagleBone Black (AM335x) Bare-Metal Startup Code
 * Target: ARM Cortex-A8 (ARMv7-A)
 * 
 * This code assumes execution begins in SVC mode after U-Boot transfers control
 * Memory Layout: Code starts at 0x80000000 (DDR)
 */

.syntax unified
.arch armv7-a
.arm

/* External symbols from linker script */
.extern _stack_top
.extern _vector_table_start
.extern _vector_table_end
.extern _bss_start
.extern _bss_end
.extern main

/* Global entry point */
.global _start

.section .text

/*
 * Entry Point: _start
 * Assumes U-Boot has transferred control in SVC mode
 * r0-r3 may contain boot parameters (we ignore them for bare-metal)
 */
_start:
    /* Disable interrupts immediately (IRQ and FIQ) */
    cpsid if
    
    /* Ensure we're in SVC mode */
    mrs r0, cpsr
    bic r0, r0, #0x1F       /* Clear mode bits */
    orr r0, r0, #0x13       /* Set SVC mode (10011) */
    msr cpsr_c, r0
    
    /*
     * Disable caches and MMU safely
     * U-Boot may have enabled them, we need to disable for bare-metal
     */
    bl disable_caches
    
    /*
     * Configure System Control Register (SCTLR) for high vectors
     * Set V bit (bit 13) to use high exception vectors at 0xFFFF0000
     */
    mrc p15, 0, r0, c1, c0, 0   /* Read SCTLR */
    orr r0, r0, #(1 << 13)      /* Set V bit for high vectors */
    bic r0, r0, #(1 << 0)       /* Clear M bit (disable MMU) */
    bic r0, r0, #(1 << 2)       /* Clear C bit (disable D-cache) */
    bic r0, r0, #(1 << 12)      /* Clear I bit (disable I-cache) */
    mcr p15, 0, r0, c1, c0, 0   /* Write SCTLR */
    isb                         /* Instruction Synchronization Barrier */
    
    /*
     * Copy vector table to high vector address (0xFFFF0000)
     * Note: On AM335x, this requires proper memory mapping setup
     * For simplicity in bare-metal, we'll set up a basic mapping
     */
    bl copy_vector_table
    
    /* Set up stack pointer for SVC mode */
    ldr sp, =_stack_top
    
    /* Initialize other mode stacks */
    bl setup_stacks
    
    /* Clear BSS section */
    bl clear_bss
    
    /* Jump to C main function */
    bl main
    
    /* If main returns, halt */
halt:
    wfi                         /* Wait for interrupt */
    b halt

/*
 * disable_caches: Safely disable all caches and MMU
 * Corrupts: r0, r1
 */
disable_caches:
    /* Disable caches in SCTLR */
    mrc p15, 0, r0, c1, c0, 0   /* Read SCTLR */
    bic r0, r0, #(1 << 0)       /* Disable MMU */
    bic r0, r0, #(1 << 2)       /* Disable D-cache */
    bic r0, r0, #(1 << 12)      /* Disable I-cache */
    mcr p15, 0, r0, c1, c0, 0   /* Write SCTLR */
    
    /* Invalidate I-cache */
    mov r0, #0
    mcr p15, 0, r0, c7, c5, 0   /* ICIALLU - Invalidate all I-cache */
    
    /* Invalidate D-cache */
    mcr p15, 0, r0, c7, c6, 0   /* DCISW - Invalidate all D-cache */
    
    /* Invalidate TLBs */
    mcr p15, 0, r0, c8, c7, 0   /* TLBIALL - Invalidate all TLBs */
    
    dsb                         /* Data Synchronization Barrier */
    isb                         /* Instruction Synchronization Barrier */
    
    bx lr

/*
 * copy_vector_table: Copy vector table to high memory
 * For bare-metal without MMU, we simulate high vectors by
 * setting up a simple mapping or copying to accessible high memory
 * Corrupts: r0, r1, r2, r3
 */
copy_vector_table:
    /* For simplicity, we'll use a CPU-specific approach
     * On AM335x, high vectors need proper memory controller setup
     * For this example, we'll set up basic high vector support */
    
    /* Get vector table source and destination */
    ldr r0, =vector_table       /* Source: our vector table */
    ldr r1, =0xFFFF0000         /* Destination: high vectors */
    mov r2, #0x20               /* Size: 8 vectors * 4 bytes each */
    
copy_loop:
    cmp r2, #0
    beq copy_done
    ldr r3, [r0], #4
    str r3, [r1], #4
    sub r2, r2, #4
    b copy_loop
    
copy_done:
    bx lr

/*
 * setup_stacks: Initialize stack pointers for different modes
 * Corrupts: r0, r1
 */
setup_stacks:
    /* Current mode is SVC, SP already set */
    
    /* IRQ mode stack */
    cps #0x12                   /* Switch to IRQ mode */
    ldr sp, =_stack_top
    sub sp, sp, #0x1000         /* IRQ stack: 4KB below SVC */
    
    /* FIQ mode stack */
    cps #0x11                   /* Switch to FIQ mode */
    ldr sp, =_stack_top
    sub sp, sp, #0x2000         /* FIQ stack: 4KB below IRQ */
    
    /* Undefined mode stack */
    cps #0x1B                   /* Switch to Undefined mode */
    ldr sp, =_stack_top
    sub sp, sp, #0x3000         /* UND stack: 4KB below FIQ */
    
    /* Abort mode stack */
    cps #0x17                   /* Switch to Abort mode */
    ldr sp, =_stack_top
    sub sp, sp, #0x4000         /* ABT stack: 4KB below UND */
    
    /* Return to SVC mode */
    cps #0x13                   /* Back to SVC mode */
    
    bx lr

/*
 * clear_bss: Zero-initialize BSS section
 * Corrupts: r0, r1, r2
 */
clear_bss:
    ldr r0, =_bss_start
    ldr r1, =_bss_end
    mov r2, #0
    
clear_loop:
    cmp r0, r1
    bge clear_done
    str r2, [r0], #4
    b clear_loop
    
clear_done:
    bx lr

/*
 * Exception vectors - will be copied to 0xFFFF0000
 * Each entry is a single instruction that branches to the handler
 */
.align 5
vector_table:
    ldr pc, =reset_handler      /* Reset */
    ldr pc, =undef_handler      /* Undefined Instruction */
    ldr pc, =swi_handler        /* Software Interrupt */
    ldr pc, =prefetch_handler   /* Prefetch Abort */
    ldr pc, =data_handler       /* Data Abort */
    nop                         /* Reserved */
    ldr pc, =irq_handler        /* IRQ */
    ldr pc, =fiq_handler        /* FIQ */

/*
 * Exception Handlers
 */
reset_handler:
    b _start

undef_handler:
    /* Undefined instruction handler */
    b undef_handler

swi_handler:
    /* Software interrupt handler */
    b swi_handler  

prefetch_handler:
    /* Prefetch abort handler */
    b prefetch_handler

data_handler:
    /* Data abort handler */
    b data_handler

/*
 * IRQ Handler - Main interrupt handler
 * This is where interrupts from AINTC will be processed
 */
irq_handler:
    /* Save context */
    sub lr, lr, #4              /* Adjust return address */
    stmfd sp!, {r0-r12, lr}     /* Save registers */
    
    /* Call C IRQ handler */
    bl c_irq_handler
    
    /* Restore context */
    ldmfd sp!, {r0-r12, pc}^    /* Restore registers and return */

fiq_handler:
    /* FIQ handler (Fast Interrupt) */
    b fiq_handler

.end