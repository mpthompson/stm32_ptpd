#ifndef __LEDS_H__
#define __LEDS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  LED_GREEN,
  LED_BLUE,
  LED_RED,
  LED_COUNT
};

void leds_init(void);
void leds_set_value(uint32_t led_index, bool led_value);
void leds_toggle_value(uint32_t led_index);

#ifdef __cplusplus
}
#endif

#endif /* __LEDS_H__ */
