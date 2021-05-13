#ifndef __TELNET_H__
#define __TELNET_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void telnet_init(void);
void telnet_flush(void);
void telnet_putc(int ch);
void telnet_puts(const char *str);
void telnet_printf(const char *fmt, ...);
int telnet_hasc(void);
int telnet_getc(void);
bool telnet_gets(char *buffer, int len, int tocase, bool echo);
bool telnet_is_thread(void);
void telnet_reboot_on_exit(void);
void telnet_shutdown_on_exit(void);

// System configurable functions to be used for network configuration.
// Implemented as weak functions.
uint16_t telnet_config_port(void);
const char *telnet_config_welcome(void);

#ifdef __cplusplus
}
#endif

#endif /* __TELNET_H__ */
