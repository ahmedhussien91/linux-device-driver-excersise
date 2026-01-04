ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 64M
}

SECTIONS
{
    /* Place vectors first at the very start of RAM */
    . = ORIGIN(RAM);

    .vectors ALIGN(32) :
    {
        KEEP(*(.vectors))
    } > RAM

    /* Collect notes AFTER vectors (or discard them) */
    .note :
    {
        *(.note*)
        *(.comment*)
    } > RAM

    .text :
    {
        *(.text*)
        *(.rodata*)
    } > RAM

    .data :
    {
        *(.data*)
    } > RAM

    .bss :
    {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    } > RAM

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        __stack_start__ = .;
        . = . + 4096;
        __stack_top__ = .;
    } > RAM
}
