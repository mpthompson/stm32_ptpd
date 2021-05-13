/**
 * @file ntshell.c
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

#include "ntshell.h"
#include "ntlibc.h"

#define VERSION_MAJOR   (0) /**< Major number. */
#define VERSION_MINOR   (2) /**< Minor number. */
#define VERSION_RELEASE (0) /**< Release number. */

/**
 * @brief Initialization code.
 */
#define INITCODE    (0x4367)

/**
 * @brief Unused variable wrapper.
 *
 * @param N A variable.
 */
#define UNUSED_VARIABLE(N)  do { (void)(N); } while (0)

/**
 * @brief Index number of the suggestion.
 *
 * @param HANDLE A pointer of the handle.
 */
#define SUGGEST_INDEX(HANDLE)   ((HANDLE)->suggest_index)

/**
 * @brief Source text string of the suggestion.
 *
 * @param HANDLE A pointer of the handle.
 */
#define SUGGEST_SOURCE(HANDLE)  ((HANDLE)->suggest_source)

/**
 * @brief Get the text editor.
 *
 * @param HANDLE A pointer of the handle.
 */
#define GET_EDITOR(HANDLE)      (&((HANDLE)->editor))

/**
 * @brief Get the text history.
 *
 * @param HANDLE A pointer of the handle.
 */
#define GET_HISTORY(HANDLE)     (&((HANDLE)->history))

/**
 * @brief Read from the serial port.
 *
 * @param HANDLE A pointer of the handle.
 * @param BUF A pointer to the buffer.
 * @param CNT Read length.
 *
 * @return The number of bytes read.
 */
#define SERIAL_READ(HANDLE, BUF, CNT)   ((HANDLE)->func_read(BUF, CNT, (HANDLE)->extobj))

/**
 * @brief Write to the serial port.
 *
 * @param HANDLE A pointer of the handle.
 * @param BUF A pointer to the buffer.
 * @param CNT Write length.
 *
 * @return The number of bytes written.
 */
#define SERIAL_WRITE(HANDLE, BUF, CNT)  ((HANDLE)->func_write(BUF, CNT, (HANDLE)->extobj))

/**
 * @brief Write the prompt to the serial port.
 *
 * @param HANDLE A pointer of the handle.
 */
#define PROMPT_WRITE(HANDLE)            SERIAL_WRITE((HANDLE), (HANDLE)->prompt, ntlibc_strlen((HANDLE)->prompt))

/**
 * @brief Write the newline to the serial port.
 *
 * @param HANDLE A pointer of the handle.
 */
#define PROMPT_NEWLINE(HANDLE)          SERIAL_WRITE((HANDLE), NTSHELL_PROMPT_NEWLINE, ntlibc_strlen(NTSHELL_PROMPT_NEWLINE))

/**
 * @brief Call the user callback function.
 *
 * @param HANDLE A pointer of the handle.
 * @param TEXT A text string for the callback function.
 */
#define CALLBACK(HANDLE, TEXT)          ((HANDLE)->func_callback((TEXT), (HANDLE)->extobj))

#define VTSEND_ERASE_LINE(HANDLE)       (vtsend_erase_line(&((HANDLE)->vtsend)))
#define VTSEND_CURSOR_HEAD(HANDLE)      (vtsend_cursor_backward(&((HANDLE)->vtsend), 80))
#define VTSEND_CURSOR_PREV(HANDLE)      (vtsend_cursor_backward(&((HANDLE)->vtsend), 1))
#define VTSEND_CURSOR_NEXT(HANDLE)      (vtsend_cursor_forward(&((HANDLE)->vtsend), 1))

/**
 * @brief Search a previous input text on the history module.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_history_prev(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    if (text_history_read_point_prev(GET_HISTORY(ntshell))) {
        char txt[TEXTHISTORY_MAXLEN];
        int n = text_history_read(GET_HISTORY(ntshell), &txt[0], sizeof(txt));
        if (0 < n) {
            VTSEND_ERASE_LINE(ntshell);
            VTSEND_CURSOR_HEAD(ntshell);
            PROMPT_WRITE(ntshell);
            SERIAL_WRITE(ntshell, txt, n);
            text_editor_set_text(GET_EDITOR(ntshell), txt);
        }
    }
}

/**
 * @brief Search a next input text on the history module.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_history_next(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    if (text_history_read_point_next(GET_HISTORY(ntshell))) {
        char txt[TEXTHISTORY_MAXLEN];
        int n = text_history_read(GET_HISTORY(ntshell), &txt[0], sizeof(txt));
        if (0 < n) {
            VTSEND_ERASE_LINE(ntshell);
            VTSEND_CURSOR_HEAD(ntshell);
            PROMPT_WRITE(ntshell);
            SERIAL_WRITE(ntshell, txt, n);
            text_editor_set_text(GET_EDITOR(ntshell), txt);
        }
    }
}

/**
 * @brief Move the cursor to left.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_cursor_left(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    if (text_editor_cursor_left(GET_EDITOR(ntshell))) {
        VTSEND_CURSOR_PREV(ntshell);
    }
}

/**
 * @brief Move the cursor to right.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_cursor_right(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    if (text_editor_cursor_right(GET_EDITOR(ntshell))) {
        VTSEND_CURSOR_NEXT(ntshell);
    }
}

/**
 * @brief Process for the enter action.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_enter(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    char txt[TEXTEDITOR_MAXLEN];
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    text_editor_get_text(GET_EDITOR(ntshell), &txt[0], sizeof(txt));
    text_editor_clear(GET_EDITOR(ntshell));
    text_history_write(GET_HISTORY(ntshell), txt);
    PROMPT_NEWLINE(ntshell);
    ntshell->exitcode = CALLBACK(ntshell, txt);
    if (ntshell->exitcode == 0) PROMPT_WRITE(ntshell);
}

/**
 * @brief Process for the cancel action.
 * @details This function change the state of the logical text editor and the view.
 *
 * @note
 * The CTRL+C operation in the general OS uses a signal.
 * In this cancel action, It simulate only the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_cancel(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    text_editor_clear(GET_EDITOR(ntshell));
    SERIAL_WRITE(ntshell, "^C", 2);
    PROMPT_NEWLINE(ntshell);
    PROMPT_WRITE(ntshell);
}

/**
 * @brief Process for the insert action.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_insert(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    UNUSED_VARIABLE(action);

    /*
     * Reject the suggestion index number if an input action occurred.
     */
    SUGGEST_INDEX(ntshell) = -1;

    /*
     * Insert the input character using the logical text editor.
     * Update the view.
     */
    if (text_editor_insert(GET_EDITOR(ntshell), ch)) {
        char txt[TEXTEDITOR_MAXLEN];
        int len = text_editor_get_text(GET_EDITOR(ntshell), &txt[0], sizeof(txt));
        int pos = text_editor_cursor_get_position(GET_EDITOR(ntshell));
        int n = len - pos;
        SERIAL_WRITE(ntshell, (char *)&ch, sizeof(ch));
        if (n > 0) {
            int i;
            SERIAL_WRITE(ntshell, txt + pos, len - pos);
            for (i = 0; i < n; i++) {
                VTSEND_CURSOR_PREV(ntshell);
            }
        }
    }
}

/**
 * @brief Process for the backspace action.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_backspace(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    if (text_editor_backspace(GET_EDITOR(ntshell))) {
        char txt[TEXTEDITOR_MAXLEN];
        int len = text_editor_get_text(GET_EDITOR(ntshell), &txt[0], sizeof(txt));
        int pos = text_editor_cursor_get_position(GET_EDITOR(ntshell));
        int n = len - pos;
        VTSEND_CURSOR_PREV(ntshell);
        if (n > 0) {
            int i;
            SERIAL_WRITE(ntshell, txt + pos, n);
            SERIAL_WRITE(ntshell, " ", 1);
            for (i = 0; i < n + 1; i++) {
                VTSEND_CURSOR_PREV(ntshell);
            }
        } else {
            SERIAL_WRITE(ntshell, " ", 1);
            VTSEND_CURSOR_PREV(ntshell);
        }
    }
}

/**
 * @brief Process for the delete action.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_delete(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    if (text_editor_delete(GET_EDITOR(ntshell))) {
        char txt[TEXTEDITOR_MAXLEN];
        int len = text_editor_get_text(GET_EDITOR(ntshell), &txt[0], sizeof(txt));
        int pos = text_editor_cursor_get_position(GET_EDITOR(ntshell));
        int n = len - pos;
        if (n > 0) {
            int i;
            SERIAL_WRITE(ntshell, txt + pos, n);
            SERIAL_WRITE(ntshell, " ", 1);
            for (i = 0; i < n + 1; i++) {
                VTSEND_CURSOR_PREV(ntshell);
            }
        } else {
            SERIAL_WRITE(ntshell, " ", 1);
            VTSEND_CURSOR_PREV(ntshell);
        }
    }
}

/**
 * @brief Process for the suggestion action.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_suggest(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    char buf[TEXTEDITOR_MAXLEN];
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    if (SUGGEST_INDEX(ntshell) < 0) {
        /*
         * Enter the input suggestion mode.
         * Get the suggested text string with the current text string.
         */
        if (text_editor_get_text(
                    GET_EDITOR(ntshell),
                    SUGGEST_SOURCE(ntshell),
                    sizeof(SUGGEST_SOURCE(ntshell))) > 0) {
            SUGGEST_INDEX(ntshell) = 0;
            if (text_history_find(
                        GET_HISTORY(ntshell),
                        SUGGEST_INDEX(ntshell),
                        SUGGEST_SOURCE(ntshell),
                        buf,
                        sizeof(buf)) == 0) {
                /*
                 * Found the suggestion.
                 */
                int n = ntlibc_strlen((const char *)buf);
                VTSEND_ERASE_LINE(ntshell);
                VTSEND_CURSOR_HEAD(ntshell);
                PROMPT_WRITE(ntshell);
                SERIAL_WRITE(ntshell, buf, n);
                text_editor_set_text(GET_EDITOR(ntshell), buf);
            } else {
                /*
                 * Not found the suggestion.
                 */
                SUGGEST_INDEX(ntshell) = -1;
            }
        }
    } else {
        /*
         * Already the input suggestion mode.
         * Search the next suggestion text string.
         */
        SUGGEST_INDEX(ntshell) = SUGGEST_INDEX(ntshell) + 1;
        if (text_history_find(
                    GET_HISTORY(ntshell),
                    SUGGEST_INDEX(ntshell),
                    SUGGEST_SOURCE(ntshell),
                    buf,
                    sizeof(buf)) == 0) {
            /*
             * Found the suggestion.
             */
            int n = ntlibc_strlen((const char *)buf);
            VTSEND_ERASE_LINE(ntshell);
            VTSEND_CURSOR_HEAD(ntshell);
            PROMPT_WRITE(ntshell);
            SERIAL_WRITE(ntshell, buf, n);
            text_editor_set_text(GET_EDITOR(ntshell), buf);
        } else {
            /*
             * Not found the suggestion.
             * Recall the previous input text string.
             */
            int n = ntlibc_strlen(SUGGEST_SOURCE(ntshell));
            VTSEND_ERASE_LINE(ntshell);
            VTSEND_CURSOR_HEAD(ntshell);
            PROMPT_WRITE(ntshell);
            SERIAL_WRITE(ntshell, SUGGEST_SOURCE(ntshell), n);
            text_editor_set_text(GET_EDITOR(ntshell), SUGGEST_SOURCE(ntshell));
            SUGGEST_INDEX(ntshell) = -1;
        }
    }
}

/**
 * @brief Move the cursor to the head of the line.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_cursor_head(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    VTSEND_CURSOR_HEAD(ntshell);
    PROMPT_WRITE(ntshell);
    text_editor_cursor_head(GET_EDITOR(ntshell));
}

/**
 * @brief Move the cursor to the tail of the line.
 * @details This function change the state of the logical text editor and the view.
 *
 * @param ntshell A handler of the NT-Shell.
 * @param action An action.
 * @param ch A input character.
 */
static void actfunc_cursor_tail(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch)
{
    char buf[TEXTEDITOR_MAXLEN];
    int len;
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(ch);
    text_editor_get_text(GET_EDITOR(ntshell), buf, sizeof(buf));
    len = ntlibc_strlen((const char *)buf);
    VTSEND_CURSOR_HEAD(ntshell);
    PROMPT_WRITE(ntshell);
    SERIAL_WRITE(ntshell, buf, len);
    text_editor_cursor_tail(GET_EDITOR(ntshell));
}

/**
 * @brief The data structure of the action table.
 * @details
 * The action consists from the state and the input character.
 * This definition also have the callback function.
 */
typedef struct {
    vtrecv_action_t action;
    unsigned char ch;
    void (*func)(ntshell_t *ntshell, vtrecv_action_t action, unsigned char ch);
} ntshell_action_table_t;

/**
 * @brief Process function table for the actions.
 * @details
 * The action codes depends on the virtual terminals.
 * So you should check some virtual terminal softwares and the environments.
 *
 * <table>
 *   <th>
 *     <td>Platform</td>
 *     <td>Tools</td>
 *   </th>
 *   <tr>
 *     <td>Windows</td>
 *     <td>Hyper Terminal, Poderossa, TeraTerm</td>
 *   </tr>
 *   <tr>
 *     <td>Linux</td>
 *     <td>minicom, screen, kermit</td>
 *   </tr>
 * </table>
 */
static const ntshell_action_table_t action_table[] = {
    {VTRECV_ACTION_EXECUTE, 0x01, actfunc_cursor_head},
    {VTRECV_ACTION_EXECUTE, 0x02, actfunc_cursor_left},
    {VTRECV_ACTION_EXECUTE, 0x03, actfunc_cancel},
    {VTRECV_ACTION_EXECUTE, 0x04, actfunc_delete},
    {VTRECV_ACTION_EXECUTE, 0x05, actfunc_cursor_tail},
    {VTRECV_ACTION_EXECUTE, 0x06, actfunc_cursor_right},
    {VTRECV_ACTION_EXECUTE, 0x08, actfunc_backspace},
    {VTRECV_ACTION_EXECUTE, 0x09, actfunc_suggest},
    {VTRECV_ACTION_EXECUTE, 0x0d, actfunc_enter},
    {VTRECV_ACTION_EXECUTE, 0x0e, actfunc_history_next},
    {VTRECV_ACTION_EXECUTE, 0x10, actfunc_history_prev},
    {VTRECV_ACTION_CSI_DISPATCH, 0x41, actfunc_history_prev},
    {VTRECV_ACTION_CSI_DISPATCH, 0x42, actfunc_history_next},
    {VTRECV_ACTION_CSI_DISPATCH, 0x43, actfunc_cursor_right},
    {VTRECV_ACTION_CSI_DISPATCH, 0x44, actfunc_cursor_left},
    {VTRECV_ACTION_CSI_DISPATCH, 0x7e, actfunc_delete},
    {VTRECV_ACTION_PRINT, 0x7f, actfunc_backspace},
};

/**
 * @brief The callback function for the vtrecv module.
 *
 * @param vtrecv The vtrecv.
 * @param action An action.
 * @param ch A character.
 */
void vtrecv_callback(vtrecv_t *vtrecv, vtrecv_action_t action, unsigned char ch)
{
    ntshell_action_table_t *p;
    int i;
    const int ACTTBLSIZ = sizeof(action_table) / sizeof(action_table[0]);

    /*
     * Search the process function for the control codes.
     */
    p = (ntshell_action_table_t *)action_table;
    for (i = 0; i < ACTTBLSIZ; i++) {
        if ((p->action == action) && (p->ch == ch)) {
            p->func(vtrecv->user_data, action, ch);
            return;
        }
        p++;
    }

    /*
     * A general character is the input character.
     */
    if (VTRECV_ACTION_PRINT == action) {
        actfunc_insert(vtrecv->user_data, action, ch);
        return;
    }

    /*
     * Other cases, there is no defined process function for the input codes.
     * If you need to support the input action, you should update the action table.
     */
}

/**
 * @brief Initialize the NT-Shell.
 *
 * @param p A pointer to the handler of NT-Shell.
 * @param func_read Serial read function.
 * @param func_write Serial write function.
 * @param func_callback Callback function.
 * @param extobj An external object for the callback function.
 */
void ntshell_init(ntshell_t *p,
    NTSHELL_SERIAL_READ func_read,
    NTSHELL_SERIAL_WRITE func_write,
    NTSHELL_USER_CALLBACK func_callback,
    void *extobj)
{
    /*
     * The vtrecv module provides a pointer interface to an external object.
     * NT-Shell uses the text editor, text history, read function, write function with the pointer interface.
     */
    p->func_read = func_read;
    p->func_write = func_write;
    p->func_callback = func_callback;
    p->extobj = extobj;
    ntlibc_strcpy(p->prompt, NTSHELL_PROMPT_DEFAULT);

    p->vtrecv.user_data = p;

    /*
     * Initialize the modules.
     */
    vtsend_init(&(p->vtsend), func_write, extobj);
    vtrecv_init(&(p->vtrecv), vtrecv_callback);
    text_editor_init(GET_EDITOR(p));
    text_history_init(GET_HISTORY(p));
    SUGGEST_INDEX(p) = -1;

    /*
     * Set the initialization code.
     */
    p->initcode = INITCODE;
    p->exitcode = 0;
}

/**
 * @brief Execute the NT-Shell.
 * @details Never return from this function.
 *
 * @param p A pointer to the handler of the NT-Shell.
 */
void ntshell_execute(ntshell_t *p)
{
    /*
     * Check the initialization code.
     */
    if (p->exitcode != 0) {
      return;
    }
    if (p->initcode != INITCODE) {
      return;
    }

    /*
     * User input loop.
     */
    PROMPT_WRITE(p);
    while (p->exitcode == 0) {
        unsigned char ch;
        if (SERIAL_READ(p, (char *)&ch, sizeof(ch)) > 0)
        {
          /*
           * Process the input character.
           */
          vtrecv_execute(&(p->vtrecv), &ch, sizeof(ch));
        }
        else
        {
          /*
           * Input stream error detected.
           */
          p->exitcode = 1;
        }
    }
}

/**
 * @brief Set up the prompt of the NT-Shell.
 *
 * @param p A pointer to the handler of the NT-Shell.
 * @param prompt A text string.
 */
void ntshell_set_prompt(ntshell_t *p, const char *prompt)
{
    /*
     * Check the initialization code.
     */
    if (p->initcode != INITCODE) {
      return;
    }

    ntlibc_strcpy(p->prompt, prompt);
}

/**
 * @brief Get the version.
 *
 * @param major Major number.
 * @param minor Minor number.
 * @param release Release number.
 */
void ntshell_version(int *major, int *minor, int *release)
{
    *major = VERSION_MAJOR;
    *minor = VERSION_MINOR;
    *release = VERSION_RELEASE;
}

