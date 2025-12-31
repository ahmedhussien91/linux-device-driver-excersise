.syntax unified
.cpu cortex-a8
.fpu vfpv3
.arm

.global _start
.section .vectors, "ax"
_start:
    b   reset

.section .text, "ax"
reset:
    /* set a simple stack */
    ldr sp, =_stack_top

    bl  main
1:  b   1b

.section .bss, "aw", %nobits
    .space 4096
_stack_top:
