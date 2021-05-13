/**
 * @file ntopt.c
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

#include "ntopt.h"

/**
 * @brief Is the character delimiter?
 * @param c The character.
 * @retval true It's a delimiter.
 * @retval false It's not a delimiter.
 */
#define IS_DELIM(c) \
    (((c) == '\r') || ((c) == '\n') || ((c) == '\t') || ((c) == '\0') || ((c) == ' '))

static int ntopt_get_count(const char *str);
static char *ntopt_get_text(
        const char *str, const int n, char *buf, int siz, int *len);

/**
 * @brief Get the sentence count of the given text string.
 * @param str A text string.
 * @return Count of the given sentence.
 */
static int ntopt_get_count(const char *str)
{
    int cnt = 0;
    int wc = 0;
    char *p = (char *)str;
    while (*p) {
        if (!IS_DELIM(*p)) {
            wc++;
            if (wc == 1) {
                cnt++;
            }
        } else {
            wc = 0;
        }
        p++;
    }
    return cnt;
}

/**
 * @brief Get the sentence of the given text string.
 * @param str A text string.
 * @param n Index number. (0 to ntopt-get_count(str) - 1)
 * @param buf The pointer to a stored buffer.
 * @param siz The size of the stored buffer.
 * @param len The stored string length.
 * @retval !NULL Success. The pointer to the buffer.
 * @retval NULL Failure.
 */
static char *ntopt_get_text(
        const char *str, const int n, char *buf, int siz, int *len)
{
    int cnt = 0;
    int wc = 0;
    char *p = (char *)str;
    *len = 0;
    while (*p) {
        if (!IS_DELIM(*p)) {
            wc++;
            if (wc == 1) {
                if (cnt == n) {
                    char *des = buf;
                    int cc = 0;
                    while (!IS_DELIM(*p)) {
                        cc++;
                        if (siz <= cc) {
                            break;
                        }
                        *des = *p;
                        des++;
                        p++;
                    }
                    *des = '\0';
                    *len = cc;
                    return buf;
                }
                cnt++;
            }
        } else {
            wc = 0;
        }
        p++;
    }
    return (char *)0;
}

/**
 * @brief Parse the given text string.
 * @param str A text string.
 * @param func The callback function.
 * @param extobj An external object for the callback function.
 * @return The return value from the callback.
 */
int ntopt_parse(const char *str, NTOPT_CALLBACK func, void *extobj)
{
    int argc;
    char argv[NTOPT_TEXT_MAXLEN];
    char *argvp[NTOPT_TEXT_MAXARGS];
    int i;
    int total;
    char *p;

    argc = ntopt_get_count(str);
    if (NTOPT_TEXT_MAXARGS <= argc) {
        argc = NTOPT_TEXT_MAXARGS;
    }

    total = 0;
    p = &argv[0];
    for (i = 0; i < argc; i++) {
        int len;
        argvp[i] = ntopt_get_text(
                str, i, p, NTOPT_TEXT_MAXLEN - total, &len);
        if (total + len + 1 < NTOPT_TEXT_MAXLEN) {
            p += len + 1;
            total += len + 1;
        } else {
            break;
        }
    }

    return func(argc, &argvp[0], extobj);
}

