#ifndef _REBOOT_H
#define _REBOOT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void reboot_system(void);
void reboot_system_no_delay(void);
void reboot_event(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _REBOOT_H */
