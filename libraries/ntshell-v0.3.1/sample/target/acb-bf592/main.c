/**
 * @file main.c
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

#include <stdlib.h>
#include "ntshell.h"
#include "chip.h"
#include "uart.h"
#include "ntlibc.h"

#define UNUSED_VARIABLE(N)  do { (void)(N); } while (0)

static int func_read(char *buf, int cnt, void *extobj)
{
  int i;
  UNUSED_VARIABLE(extobj);
  for (i = 0; i < cnt; i++) {
    buf[i] = uart_getc();
  }
  return cnt;
}

static int func_write(const char *buf, int cnt, void *extobj)
{
  int i;
  UNUSED_VARIABLE(extobj);
  for (i = 0; i < cnt; i++) {
    uart_putc(buf[i]);
  }
  return cnt;
}

static int func_callback(const char *text, void *extobj)
{
  ntshell_t *ntshell = (ntshell_t *)extobj;
  UNUSED_VARIABLE(ntshell);
  UNUSED_VARIABLE(extobj);
  if (ntlibc_strlen(text) > 0) {
    uart_puts("User input text:'");
    uart_puts(text);
    uart_puts("'\r\n");
  }
  return 0;
}

int main(void)
{
  ntshell_t ntshell;
  chip_init();
  uart_init();
  ntshell_init(
      &ntshell,
      func_read,
      func_write,
      func_callback,
      (void *)&ntshell);
  ntshell_set_prompt(&ntshell, "BlueTank>");
  ntshell_execute(&ntshell);
  return 0;
}

