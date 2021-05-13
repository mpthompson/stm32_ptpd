#ifndef __BLINK_H__
#define __BLINK_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void blink_init(void);
void blink_set_rate(uint32_t blinks_per_second);

// System configurable functions. Implemented as weak functions.
uint32_t blink_config_led(void);
uint32_t blink_config_rate(void);

#ifdef __cplusplus
}
#endif

#endif /* __BLINK_H__ */
