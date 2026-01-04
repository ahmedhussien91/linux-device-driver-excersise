#include <stdint.h>

#define REG32(a) (*(volatile uint32_t *)(a))

#define UART0_BASE  0x44E09000

#define UART_THR    (UART0_BASE + 0x00)
#define UART_LSR    (UART0_BASE + 0x14)
#define UART_LCR    (UART0_BASE + 0x0C)
#define UART_DLL    (UART0_BASE + 0x00)
#define UART_DLH    (UART0_BASE + 0x04)

static inline void uart_wait_tx(void)
{
    /* THR empty */
    while (!(REG32(UART_LSR) & (1 << 5)));
}

void uart_putc(char c)
{
    uart_wait_tx();
    REG32(UART_THR) = c;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

void uart_init(void)
{
    /* 115200 baud assuming 48MHz UART clock */
    REG32(UART_LCR) = 0x80;  /* DLAB = 1 */
    REG32(UART_DLL) = 26;    /* divisor */
    REG32(UART_DLH) = 0;
    REG32(UART_LCR) = 0x03;  /* 8N1 */

    uart_puts("\n[UART] init OK\n");
}
