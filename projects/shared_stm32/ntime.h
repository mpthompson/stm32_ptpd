#ifndef _NTIME_H
#define _NTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ntime_init(uint32_t timer_priority);
void ntime_suspend(void);
void ntime_resume(void);
int64_t ntime_get_nsecs(void);
uint32_t ntime_get_msecs(void);
void ntime_to_secs(int64_t timestamp, int32_t *seconds, int32_t *nanoseconds);

#ifdef __cplusplus
}
#endif

#endif /* _NTIME_H */
