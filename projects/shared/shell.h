#ifndef __SHELL_H__
#define __SHELL_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Shell client function types.
typedef int (*shell_hasc_t)(void);
typedef int (*shell_getc_t)(void);
typedef void (*shell_putc_t)(int ch);
typedef void (*shell_flush_t)(void);
typedef void (*shell_reboot_t)(void);
typedef void (*shell_shutdown_t)(void);

// Shell client structure.
typedef struct shell_client_s
{
  void *thread_id;
  shell_hasc_t hasc;
  shell_getc_t getc;
  shell_putc_t putc;
  shell_flush_t flush;
  shell_reboot_t reboot;
  shell_shutdown_t shutdown;
} shell_client_t;

// Shell command function type.
typedef bool (*shell_func_t)(int argc, char **argv);

void shell_init(void);
void shell_parse_arguments(char *cmdline, int *argc, char *argv[], int argv_count);
bool shell_command(char *cmdline);
int shell_add_client(shell_client_t *client);
int shell_add_command(const char *name, shell_func_t func);
void shell_process(void);
int shell_hasc(void);
int shell_getc(void);
void shell_putc_raw(int c);
void shell_putc(int c);
void shell_puts(const char *str);
void shell_printf(const char *fmt, ...);
void shell_pdump(const void *buffer, size_t buflen, uint32_t address, uint32_t width);
void shell_flush(void);

// System configurable functions to be used for network configuration.
// Implemented as weak functions.
const char *shell_config_version(void);

#ifdef __GNUC__
// GNUC includes strcasecmp() in strings.h, not string.h.  We
// define the external function here to simplify cross compilation
// between ARMCC and GNUC.
extern int strcasecmp(const char *, const char *);
extern int strncasecmp(const char *, const char *, size_t);
#endif // __GNUC__

#ifdef __cplusplus
}
#endif

#endif /* __SHELL_H__ */
