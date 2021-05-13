#ifndef __TRIGOUT_H__
#define __TRIGOUT_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void trigout_init(void);
bool trigout_get_output(void);
void trigout_set_output(bool value);
void trigout_pulse(uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif  // __TRIGOUT_H__
