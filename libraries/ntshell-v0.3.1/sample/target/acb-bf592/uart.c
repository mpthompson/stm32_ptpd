/**
 * @file uart.c
 * @author CuBeatSystems
 * @author Shinichiro Nakamura
 * @copyright
 * ===============================================================
 * Natural Tiny Shell (NT-Shell) Version 0.3.0
 * ===============================================================
 * Copyright (c) 2010-2015 Shinichiro Nakamura
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cdefBF592-A.h>
#include <builtins.h>
#include "uart.h"
#include "chip.h"

#define UART0_BAUDRATE  (57600)
#define UART0_DIVISOR   (CHIP_SYSCLK_HZ / 16 / UART0_BAUDRATE)

void uart_init(void)
{
    *pUART0_GCTL = 1;

    *pUART0_LCR |= DLAB;
    *pUART0_DLL = UART0_DIVISOR & 0xFF;
    *pUART0_DLH = UART0_DIVISOR >> 8;
    *pUART0_LCR &= ~DLAB;

    *pUART0_LCR = WLS(8);

    *pUART0_IER = 0;

    *pPORTF_MUX &= ~(PF11 | PF12);
    *pPORTF_FER |=  (PF11 | PF12);
}

int uart_putc(char c)
{
    while (0 == (*pUART0_LSR & THRE)) {
        ssync();
    }
    *pUART0_THR = c;
    ssync();
    return c;
}

int uart_getc(void)
{
    while (0 == (*pUART0_LSR & DR)) {
        ssync();
    }
    return *pUART0_RBR;
}

int uart_puts(const char *s)
{
    int cnt = 0;
    char *p = (char *)s;
    while (*p) {
        uart_putc(*p);
        p++;
        cnt++;
    }
    return cnt;
}

