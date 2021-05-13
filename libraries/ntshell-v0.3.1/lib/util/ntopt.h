/**
 * @file ntopt.h
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

#ifndef NTOPT_H
#define NTOPT_H

#include "ntconf.h"

/**
 * @brief The text maximum length.
 */
#define NTOPT_TEXT_MAXLEN   (NTCONF_EDITOR_MAXLEN)

/**
 * @brief The text maximum arguments.
 */
#define NTOPT_TEXT_MAXARGS  (NTCONF_EDITOR_MAXLEN / 2)

/**
 * @brief The callback function.
 *
 * @param argc The number of the parameters.
 * @param argv The pointer to the parameters.
 * @param extobj The external object.
 *
 * @return A return value.
 */
typedef int (*NTOPT_CALLBACK)(int argc, char **argv, void *extobj);

#ifdef __cplusplus
extern "C" {
#endif

int ntopt_parse(const char *str, NTOPT_CALLBACK func, void *extobj);

#ifdef __cplusplus
}
#endif

#endif

