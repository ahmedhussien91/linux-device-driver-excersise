/*
 * Linker Script for BeagleBone Black (AM335x) Bare-Metal Application
 * Target: ARM Cortex-A8 (ARMv7-A)
 * Memory Layout: Code in DDR starting at 0x80000000
 */

ENTRY(_start)

MEMORY
{
    /* DDR3 RAM on BeagleBone Black: 512MB starting at 0x80000000 
     * U-Boot typically loads at 0x80008000, but we'll use 0x80000000
     * to have full control over memory layout */
    DDR : ORIGIN = 0x80000000, LENGTH = 512M
    
    /* High Vector Table area (remapped to 0xFFFF0000 by MMU/SCTLR) */
    VECTORS : ORIGIN = 0xFFFF0000, LENGTH = 0x1000
}

SECTIONS
{
    /* Reset vector and startup code at load address */
    .text 0x80000000 : {
        *startup.o(.text)
        *vectors.o(.text)
        *(.text*)
        *(.rodata*)
    } > DDR

    /* Data section */
    .data : {
        _data_start = .;
        *(.data*)
        _data_end = .;
    } > DDR

    /* BSS section (uninitialized data) */
    .bss : {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    } > DDR

    /* Stack definition */
    .stack : {
        . = ALIGN(8);
        _stack_start = .;
        . = . + 0x10000;  /* 64KB stack */
        _stack_end = .;
    } > DDR

    /* Vector table copy destination */
    .vectors 0xFFFF0000 : AT(ADDR(.text) + SIZEOF(.text)) {
        _vector_table_start = .;
        *(.vectors)
        _vector_table_end = .;
    } > VECTORS

    /* Heap (if needed) */
    .heap : {
        _heap_start = .;
        . = . + 0x100000;  /* 1MB heap */
        _heap_end = .;
    } > DDR
}

/* Exported symbols for startup code */
_stack_top = _stack_end;
_vector_table_size = _vector_table_end - _vector_table_start;