/**
 * @file ntstdio.c
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

#include <stdarg.h>
#include "ntstdio.h"

#define FLAG_ZERO_PADDED    (1 << 0)
#define FLAG_LEFT_JUSTIFIED (1 << 1)
#define FLAG_SIZE_LONG_INT  (1 << 2)
#define FLAG_SIGNED_DECIMAL (1 << 3)

/*
 * ntstdio_printf("%d", 1234);            "1234"
 * ntstdio_printf("%6d,%3d%%", -200, 5);  "  -200,  5%"
 * ntstdio_printf("%-6u", 100);           "100   "
 * ntstdio_printf("%ld", 12345678L);      "12345678"
 * ntstdio_printf("%04x", 0xA3);          "00a3"
 * ntstdio_printf("%08LX", 0x123ABC);     "00123ABC"
 * ntstdio_printf("%016b", 0x550F);       "0101010100001111"
 * ntstdio_printf("%s", "String");        "String"
 * ntstdio_printf("%-4s", "abc");         "abc "
 * ntstdio_printf("%4s", "abc");          " abc"
 * ntstdio_printf("%c", 'a');             "a"
 * ntstdio_printf("%f", 10.0);            <ntstdio_printf lacks floating point support>
 */

static void xvprintf(ntstdio_t *handle, const char *fmt, va_list arp)
{
    unsigned int i, j;
    unsigned int flag, radix, width;
    unsigned long value;
    char s[16], c, d, *p;

    while (1) {
        /*
         * Get a character.
         */
        c = *fmt++;
        if (!c) {
            /*
             * End of the format.
             */
            break;
        }
        if (c != '%') {
            /*
             * Pass through it if not a % sequense
             */
            ntstdio_putc(handle, c);
            continue;
        }

        /*
         * Reset the flag.
         */
        flag = 0;

        /*
         * Get the first character of the sequense.
         */
        c = *fmt++;
        if (c == '0') {
            flag = FLAG_ZERO_PADDED;
            c = *fmt++;
        } else {
            if (c == '-') {
                flag = FLAG_LEFT_JUSTIFIED;
                c = *fmt++;
            }
        }
        /*
         * Calculate the minimum width.
         */
        for (width = 0; (c >= '0') && (c <= '9'); c = *fmt++) {
            width = (width * 10) + (c - '0');
        }
        if ((c == 'l') || (c == 'L')) {
            flag |= FLAG_SIZE_LONG_INT;
            c = *fmt++;
        }
        if (!c) {
            /*
             * End of the format.
             */
            break;
        }
        d = c;
        if (d >= 'a') {
            d -= 0x20;
        }
        /* Type is... */
        switch (d) {
            case 'S' :
                /* String */
                p = va_arg(arp, char*);
                for (j = 0; p[j]; j++) {
                }
                while (!(flag & FLAG_LEFT_JUSTIFIED) && (j++ < width)) {
                    ntstdio_putc(handle, ' ');
                }
                ntstdio_puts(handle, p);
                while (j++ < width) {
                    ntstdio_putc(handle, ' ');
                }
                continue;
            case 'C' :
                /* Character */
                ntstdio_putc(handle, (char)va_arg(arp, int));
                continue;
            case 'B' :
                /* Binary */
                radix = 2;
                break;
            case 'O' :
                /* Octal */
                radix = 8;
                break;
            case 'D' :
                /* Signed decimal */
                radix = 10;
                break;
            case 'U' :
                /* Unsigned decimal */
                radix = 10;
                break;
            case 'X' :
                /* Hexdecimal */
                radix = 16;
                break;
            default:
                /* Unknown type (passthrough) */
                ntstdio_putc(handle, c);
                continue;
        }

        /*
         * Get an argument and put it in numeral.
         */
        value = (flag & FLAG_SIZE_LONG_INT) ? va_arg(arp, long) : ((d == 'D') ? (long)va_arg(arp, int) : (long)va_arg(arp, unsigned int));
        if ((d == 'D') && (value & 0x80000000)) {
            value = 0 - value;
            flag |= FLAG_SIGNED_DECIMAL;
        }
        i = 0;
        do {
            d = (char)(value % radix);
            value /= radix;
            if (d > 9) {
                d += (c == 'x') ? 0x27 : 0x07;
            }
            s[i++] = d + '0';
        } while (value && (i < sizeof(s)));
        if (flag & FLAG_SIGNED_DECIMAL) {
            s[i++] = '-';
        }
        j = i;
        d = (flag & FLAG_ZERO_PADDED) ? '0' : ' ';
        while (!(flag & FLAG_LEFT_JUSTIFIED) && (j++ < width)) {
            ntstdio_putc(handle, d);
        }
        do {
            ntstdio_putc(handle, s[--i]);
        } while(i);
        while (j++ < width) {
            ntstdio_putc(handle, ' ');
        }
    }
}

void ntstdio_init(ntstdio_t *handle, unsigned int option, NTSTDIO_XI xi, NTSTDIO_XO xo)
{
    handle->xi = xi;
    handle->xo = xo;
    handle->outptr = 0;
    handle->option = option;
}

void ntstdio_putc(ntstdio_t *handle, char c)
{
    if ((handle->option & NTSTDIO_OPTION_CR_CRLF) && (c == '\n')) {
        ntstdio_putc(handle, '\r');
    }

    if (handle->outptr) {
        *(handle->outptr)++ = (unsigned char)c;
        return;
    }

    if (handle->xo) {
        handle->xo((unsigned char)c);
    }
}

void ntstdio_puts(ntstdio_t *handle, const char *str)
{
    while (*str) {
        ntstdio_putc(handle, *str++);
    }
}

void ntstdio_fputs(ntstdio_t *handle, NTSTDIO_XO xo, const char *str)
{
    void (*pf)(unsigned char);

    /* Save current output device */
    pf = handle->xo;
    /* Switch output to specified device */
    handle->xo = xo;

    while (*str) {
        ntstdio_putc(handle, *str++);
    }

    /* Restore output device */
    handle->xo = pf;
}

void ntstdio_printf(ntstdio_t *handle, const char *fmt, ...)
{
    va_list arp;
    va_start(arp, fmt);
    xvprintf(handle, fmt, arp);
    va_end(arp);
}

void ntstdio_sprintf(ntstdio_t *handle, char *buf, const char *fmt, ...)
{
    va_list arp;
    /* Switch destination for memory */
    handle->outptr = buf;
    va_start(arp, fmt);
    xvprintf(handle, fmt, arp);
    va_end(arp);

    /* Terminate output string with a \0 */
    *(handle->outptr) = 0;
    /* Switch destination for device */
    handle->outptr = 0;
}

void ntstdio_fprintf(ntstdio_t *handle, NTSTDIO_XO xo, const char *fmt, ...)
{
    va_list arp;
    void (*pf)(unsigned char);

    /* Save current output device */
    pf = handle->xo;
    /* Switch output to specified device */
    handle->xo = xo;

    va_start(arp, fmt);
    xvprintf(handle, fmt, arp);
    va_end(arp);

    /* Restore output device */
    handle->xo = pf;
}

/* 0:End of stream, 1:A line arrived */
int ntstdio_gets(ntstdio_t *handle, char *buf, int len)
{
    int c, i;

    if (!handle->xi) {
        /* No input function specified */
        return 0;
    }

    i = 0;
    for (;;) {
        /* Get a char from the incoming stream */
        c = handle->xi();
        if (!c) {
            /* End of stream */
            return 0;
        }
        if (c == '\r') {
            /* End of line */
            break;
        }
        if ((c == '\b') && i) {
            /* Back space */
            i--;
            if (handle->option & NTSTDIO_OPTION_LINE_ECHO) {
                ntstdio_putc(handle, c);
            }
            continue;
        }
        if ((c >= ' ') && (i < len - 1)) {
            /* Visible chars */
            buf[i++] = c;
            if (handle->option & NTSTDIO_OPTION_LINE_ECHO) {
                ntstdio_putc(handle, c);
            }
        }
    }
    buf[i] = 0;
    /* Terminate with a \0 */
    if (handle->option & NTSTDIO_OPTION_LINE_ECHO) {
        ntstdio_putc(handle, '\n');
    }
    return 1;
}

/* 0:End of stream, 1:A line arrived */
int ntstdio_fgets(ntstdio_t *handle, NTSTDIO_XI xi, char *buf, int len)
{
    unsigned char (*pf)(void);
    int n;

    /* Save current input device */
    pf = handle->xi;
    /* Switch input to specified device */
    handle->xi = xi;
    /* Get a line */
    n = ntstdio_gets(handle, buf, len);
    /* Restore input device */
    handle->xi = pf;

    return n;
}

