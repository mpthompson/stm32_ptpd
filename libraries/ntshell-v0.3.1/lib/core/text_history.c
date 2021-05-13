/**
 * @file text_history.c
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

#include "text_history.h"
#include "ntlibc.h"

/**
 * @brief Initialize this module.
 *
 * @param p A pointer to the handler.
 */
void text_history_init(text_history_t *p)
{
    int i;
    p->rp = 0;
    p->wp = 0;
    for (i = 0; i < (int)sizeof(p->history); i++) {
        p->history[i] = 0;
    }
}

/**
 * @brief Write to the history.
 *
 * @param p A pointer to the handler.
 * @param buf A pointer to the buffer.
 */
int text_history_write(text_history_t *p, char *buf)
{
    char *sp = p->history + (TEXTHISTORY_MAXLEN * p->wp);
    if (buf[0] == '\0') {
        return 0;
    }
    while (*buf) {
        *sp = *buf;
        sp++;
        buf++;
    }
    *sp = '\0';
    p->wp = (p->wp + 1) % TEXTHISTORY_DEPTH;
    p->rp = p->wp;
    return 1;
}

/**
 * @brief Read from the history.
 *
 * @param p A pointer to the handler.
 * @param buf A pointer to the buffer.
 * @param siz A size of the buffer.
 */
int text_history_read(text_history_t *p, char *buf, const int siz)
{
    char *sp = p->history + (TEXTHISTORY_MAXLEN * p->rp);
    int n = 0;
    while (*sp) {
        *buf = *sp;
        buf++;
        sp++;
        n++;
        if (siz - 1 <= n) {
            break;
        }
    }
    *buf = '\0';
    return n;
}

/**
 * @brief Change the pointing location to the next.
 *
 * @param p A pointer to the handler.
 */
int text_history_read_point_next(text_history_t *p)
{
    int n = (p->rp + 1) % TEXTHISTORY_DEPTH;
    if (n != p->wp) {
        p->rp = n;
        return 1;
    }
    return 0;
}

/**
 * @brief Change the pointing location to the previous.
 *
 * @param p A pointer to the handler.
 */
int text_history_read_point_prev(text_history_t *p)
{
    int n = (p->rp == 0) ? (TEXTHISTORY_DEPTH - 1) : (p->rp - 1);
    if (n != p->wp) {
        char *sp = p->history + (TEXTHISTORY_MAXLEN * n);
        if (*sp != '\0') {
            p->rp = n;
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Find the given text string from the text history.
 *
 * @param p A pointer to the handler.
 * @param index An index number of the history.
 * @param text The target text string.
 * @param buf A pointer to the buffer.
 * @param siz A size of the buffer.
 *
 * @retval 0 Success.
 * @retval !0 Failure.
 */
int text_history_find(text_history_t *p,
        const int index, const char *text,
        char *buf, const int siz)
{
    const int text_len = ntlibc_strlen((const char *)text);
    int found = 0;
    int i;
    for (i = 0; i < TEXTHISTORY_DEPTH; i++) {
        int target = (p->rp + i) % TEXTHISTORY_DEPTH;
        char *txtp = p->history + (TEXTHISTORY_MAXLEN * target);
        const int target_len = ntlibc_strlen((const char *)txtp);
        int comp_len = (target_len < text_len) ? target_len : text_len;
        if ((ntlibc_strncmp(
                    (const char *)txtp,
                    (const char *)text, comp_len) == 0) && (comp_len > 0)) {
            if (found == index) {
                if (siz <= ntlibc_strlen(txtp)) {
                    return -1;
                }
                ntlibc_strcpy((char *)buf, (char *)txtp);
                return 0;
            }
            found++;
        }
    }
    return -1;
}

