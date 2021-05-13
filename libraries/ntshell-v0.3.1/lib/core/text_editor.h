/**
 * @file text_editor.h
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

#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include "ntconf.h"

/**
 * @brief Maximum length of the text string.
 */
#define TEXTEDITOR_MAXLEN   (NTCONF_EDITOR_MAXLEN)

/**
 * @brief Text editor handler.
 */
typedef struct {
    char buffer[TEXTEDITOR_MAXLEN]; /**< Buffer for the text string. */
    int pos;                        /**< Position of the logical cursor. */
    int len;                        /**< Length of the text string. */
} text_editor_t;

#ifdef __cplusplus
extern "C" {
#endif

void text_editor_init(text_editor_t *p);
int text_editor_insert(text_editor_t *p, char c);
int text_editor_backspace(text_editor_t *p);
int text_editor_delete(text_editor_t *p);
int text_editor_cursor_get_position(text_editor_t *p);
int text_editor_cursor_head(text_editor_t *p);
int text_editor_cursor_tail(text_editor_t *p);
int text_editor_cursor_left(text_editor_t *p);
int text_editor_cursor_right(text_editor_t *p);
int text_editor_set_text(text_editor_t *p, char *buf);
int text_editor_get_text(text_editor_t *p, char *buf, int siz);
void text_editor_clear(text_editor_t *p);

#ifdef __cplusplus
}
#endif

#endif

