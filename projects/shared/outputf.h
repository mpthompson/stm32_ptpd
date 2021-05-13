#ifndef _OUTPUTF_H
#define _OUTPUTF_H

#include <stddef.h>
#include <stdarg.h>

// Outputf putc function pointer.
typedef void (*putc_func_t)(int);
typedef void (*putc_func_with_arg_t)(int, void *arg);

int outputf(const char *fmt, ...);
int voutputf(const char *fmt, va_list args);
int soutputf(char *s, const char *fmt, ...);
int vsoutputf(char *s, const char *fmt, va_list args);
int snoutputf(char *buffer, size_t buflen, const char *fmt, ...);
int vsnoutputf(char *buffer, size_t buflen, const char *fmt, va_list args);
int foutputf(putc_func_t func, const char *fmt, ...);
int vfoutputf(putc_func_t func, const char *fmt, va_list args);
int faoutputf(putc_func_with_arg_t func, void *func_arg, const char *fmt, ...);
int vafoutputf(putc_func_with_arg_t func, void *func_arg, const char *fmt, va_list args);
int sinputf(const char *buffer, const char *fmt, ...);
int vsinputf(const char *buffer, const char *fmt, va_list args);

#endif /* _OUTPUTF_H */
