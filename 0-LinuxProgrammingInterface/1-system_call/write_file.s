.section .data
fd:     .word 1                // File descriptor (stdout)
filename:   .asciz "output.txt"
text:       .asciz "Hello, World!\n"
text_len:   .word 14

.section .bss
.lcomm buffer, 1024            // Allocate 1024 bytes for buffer

.section .text
.global _start

_start:
    // Open the file (syscall number 5)
    mov r7, #5                  // syscall number for sys_open
    ldr r0, =filename           // filename
    mov r1, #0101               // O_WRONLY | O_CREAT | O_TRUNC
    mov r2, #0600               // S_IRUSR | S_IWUSR
    svc #0
    ldr r1, =fd                 // load address of fd
    str r0, [r1]                  // store the file descriptor

    // Write to the file (syscall number 4)
    mov r7, #4                  // syscall number for sys_write
    ldr r0, =fd                 // file descriptor
    ldr r0, [r0]
    ldr r1, =text               // buffer
    ldr r2, =text_len           // buffer length
    ldr r2, [r2]
    svc #0

    // Close the file (syscall number 6)
    mov r7, #6                  // syscall number for sys_close
    ldr r0, =fd                 // file descriptor
    ldr r0, [r0]
    svc #0

    // Exit (syscall number 1)
    mov r7, #1                  // syscall number for sys_exit
    mov r0, #0                  // exit code 0
    svc #0
