#ifndef _EVENT_H
#define _EVENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*event_t)(void *arg);

void event_init(void);
void event_loop(void);
int event_schedule(event_t handler, void *arg, uint32_t timeout);
int event_schedule_1hz(event_t handler, void *arg);
int event_cancel_1hz(event_t event_handler);

#ifdef __cplusplus
}
#endif

#endif /* _EVENT_H */
