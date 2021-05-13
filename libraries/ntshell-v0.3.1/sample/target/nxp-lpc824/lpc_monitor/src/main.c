/**
 * @file main.c
 * @author CuBeatSystems
 * @author Shinichiro Nakamura
 * @copyright
 * ===============================================================
 * Natural Tiny Shell (NT-Shell) Version 0.3.1
 * ===============================================================
 * Copyright (c) 2010-2016 Shinichiro Nakamura
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

#include "chip.h"
#include "uart.h"
#include "ntshell.h"
#include "usrcmd.h"
#include <cr_section_macros.h>

static int serial_read(char *buf, int cnt, void *extobj)
{
    for (int i = 0; i < cnt; i++) {
        buf[i] = uart_getc();
    }
    return cnt;
}

static int serial_write(const char *buf, int cnt, void *extobj)
{
    for (int i = 0; i < cnt; i++) {
        uart_putc(buf[i]);
    }
    return cnt;
}

static int user_callback(const char *text, void *extobj)
{
#if 0
    /*
     * This is a really simple example codes for the callback function.
     */
    uart_puts("USERINPUT[");
    uart_puts(text);
    uart_puts("]\r\n");
#else
    /*
     * This is a complete example for a real embedded application.
     */
    usrcmd_execute(text);
#endif
    return 0;
}

int main(void)
{
    void *extobj = 0;
    ntshell_t nts;
    SystemCoreClockUpdate();
    uart_init();
    uart_puts("User command example for NT-Shell.\r\n");
    ntshell_init(&nts, serial_read, serial_write, user_callback, extobj);
    ntshell_set_prompt(&nts, "LPC824>");
    while (1) {
        ntshell_execute(&nts);
    }
    return 0 ;
}

