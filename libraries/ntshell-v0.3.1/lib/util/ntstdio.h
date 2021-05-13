/**
 * @file ntstdio.h
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

/*------------------------------------------------------------------------/
/  Universal string handler for user console interface
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2011, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

#ifndef NTSTDIO_H
#define NTSTDIO_H

/**
 * 1: Convert \n ==> \r\n in the output char.
 */
#define NTSTDIO_OPTION_CR_CRLF      (1 << 0)

/**
 * 1: Echo back input chars in ntstdio_gets function.
 */
#define NTSTDIO_OPTION_LINE_ECHO    (1 << 1)

typedef unsigned char (*NTSTDIO_XI)(void);
typedef void (*NTSTDIO_XO)(unsigned char c);

typedef struct {
    NTSTDIO_XI xi;
    NTSTDIO_XO xo;
    char *outptr;
    unsigned int option;
} ntstdio_t;

void ntstdio_init(ntstdio_t *handle, unsigned int option, NTSTDIO_XI xi, NTSTDIO_XO xo);

void ntstdio_putc(ntstdio_t *handle, char c);
void ntstdio_puts(ntstdio_t *handle, const char *str);
void ntstdio_fputs(ntstdio_t *handle, NTSTDIO_XO xo, const char *str);
void ntstdio_printf(ntstdio_t *handle, const char *fmt, ...);
void ntstdio_sprintf(ntstdio_t *handle, char *buf, const char *fmt, ...);
void ntstdio_fprintf(ntstdio_t *handle, NTSTDIO_XO xo, const char *fmt, ...);

int ntstdio_gets(ntstdio_t *handle, char *buf, int len);
int ntstdio_fgets(ntstdio_t *handle, NTSTDIO_XI xi, char *buf, int len);

#endif

