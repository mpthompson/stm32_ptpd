#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void random_init(void);
uint32_t random_get(void);
void random_delay(uint32_t min_ms, uint32_t max_ms);

#ifdef __cplusplus
}
#endif

#endif /* __RANDOM_H__ */
