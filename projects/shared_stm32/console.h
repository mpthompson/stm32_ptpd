#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void console_init(void);
void console_deinit(void);
void console_shell_init(void);
bool console_gets(char *buffer, int len, int tocase, bool echo);
int console_hasc(void);
int console_getc(void);
void console_puts(const char *str);
void console_putc(int ch);
void console_putc_interrupt(int ch);
void console_put_buf(char *buffer, int len);
void console_flush(void);
void console_reboot(void);
void console_shutdown(void);

// System configurable functions. Implemented as weak functions.
uint32_t console_config_baudrate(void);
uint32_t console_config_stop_bits(void);
uint32_t console_config_parity(void);
uint32_t console_config_preempt_priority(void);

#ifdef __cplusplus
}
#endif

#endif /* __CONSOLE_H__ */
