#ifndef __LOG_H__
#define __LOG_H__

// Initialize log buffering.
void log_init(void);
void log_putc(int ch);
void log_puts(const char *str);
void log_printf(const char *fmt, ...);
void log_dump_to_shell(void);

#endif /* __LOG_H__ */
