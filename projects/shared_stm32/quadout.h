#ifndef __QUADOUT_H__
#define __QUADOUT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void quadout_init(void);
void quadout_set_frequency(uint32_t freq);
uint32_t quadout_get_frequency();

#ifdef __cplusplus
}
#endif

#endif  // __QUADOUT_H__
