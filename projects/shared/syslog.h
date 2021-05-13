#ifndef __SYSLOG_H__
#define __SYSLOG_H__

// BSD Syslog Protocol
// See: https://www.ietf.org/rfc/rfc3164.txt

#include <stdint.h>
#include <stdbool.h>
#include "lwip/ip4_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Syslog facility values.
enum
{
  SYSLOG_KERNEL = 0,        // kernel messages
  SYSLOG_USERLEVEL,         // user-level messages
  SYSLOG_MAIL,              // mail system
  SYSLOG_SYSTEM,            // system daemons
  SYSLOG_SECURITY,          // security/authorization messages
  SYSLOG_INTERNAL,          // messages generated internally by syslogd
  SYSLOG_PRINTER,           // line printer subsystem
  SYSLOG_NEWS,              // network news subsystem
  SYSLOG_UUCP,              // UUCP subsystem
  SYSLOG_CLOCK,             // clock daemon
  SYSLOG_AUTH,              // security/authorization messages
  SYSLOG_FTP,               // FTP daemon
  SYSLOG_NTP,               // NTP subsystem
  SYSLOG_LOGAUDIT,          // log audit
  SYSLOG_LOGALERT,          // log alert
  SYSLOG_CLOCK2,            // clock daemon
  SYSLOG_LOCAL0,            // local use 0  (local0)
  SYSLOG_LOCAL1,            // local use 1  (local1)
  SYSLOG_LOCAL2,            // local use 2  (local2)
  SYSLOG_LOCAL3,            // local use 3  (local3)
  SYSLOG_LOCAL4,            // local use 4  (local4)
  SYSLOG_LOCAL5,            // local use 5  (local5)
  SYSLOG_LOCAL6,            // local use 6  (local6)
  SYSLOG_LOCAL7             // local use 7  (local7)
};

// Syslog severity values.
enum
{
  SYSLOG_EMERGENCY = 0,     // Emergency: system is unusable
  SYSLOG_ALERT,             // Alert: action must be taken immediately
  SYSLOG_CRITICAL,          // Critical: critical conditions
  SYSLOG_ERROR,             // Error: error conditions
  SYSLOG_WARNING,           // Warning: warning conditions
  SYSLOG_NOTICE,            // Notice: normal but significant condition
  SYSLOG_INFO,              // Informational: informational messages
  SYSLOG_DEBUG              // Debug: debug-level messages
};

void syslog_init(void);
void syslog_clear(void);
void syslog_set_echo(bool enable);
bool syslog_get_echo(void);
void syslog_set_level(int severity);
int syslog_get_level(void);
void syslog_printf(int severity, const char *fmt, ...);

// System configurable functions to be used for network configuration.
// Implemented as weak functions.
ip4_addr_t syslog_config_address(void);
uint16_t syslog_config_port(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSLOG_H__ */
