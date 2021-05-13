#ifndef _DELAY_H_
#define _DELAY_H_

#include <stdint.h>
#include <stdbool.h>
#include "tick.h"
#include "random.h"
#include "hal_system.h"

#ifdef __cplusplus
extern "C" {
#endif

void delay_init(void);

// RTOS based delay in milliseconds inline helper function.
__STATIC_INLINE int delay_ms(uint32_t milliseconds)
{
  return osDelay(tick_from_milliseconds(milliseconds)) == osOK ? 0 : -1;
}

// RTOS based delay microseconds inline helper function.
__STATIC_INLINE int delay_us(uint32_t microseconds)
{
  return osDelay(tick_from_microseconds(microseconds)) == osOK ? 0 : -1;
}

// Delay milliseconds for a random time.
__STATIC_INLINE int delay_random_ms(uint32_t min_ms, uint32_t max_ms)
{
  // Sanity check minimum and maximum milliseconds.
  if (max_ms < min_ms)
  {
    // Swap the two values.
    max_ms = max_ms + min_ms;
    min_ms = max_ms - min_ms;
    max_ms = max_ms - min_ms;
  }

  // Make sure the difference is not zero.
  if (min_ms == max_ms) max_ms = max_ms + 1;

  // Now delay for a period between the minimum and maximum delay.
  return delay_ms(min_ms + (random_get() % (max_ms - min_ms)));
}

// More precise microsecond delay using fast DWT hardware counter in STM32 debug unit.
// This should only be used for short delays on the order of a few microseconds.
// See: https://community.st.com/s/question/0D50X00009XkeRYSAZ/delay-in-us
__STATIC_INLINE void delay_dwt_us(volatile uint32_t microseconds)
{
  // Get the start time.
  uint32_t clk_cycle_start = DWT->CYCCNT;

  // Adjust microsecond count based on HLC frequency.
  microseconds *= (HAL_RCC_GetHCLKFreq() / 1000000);

  // Delay until the end.
  while ((DWT->CYCCNT - clk_cycle_start) < microseconds);
}

#ifdef __cplusplus
}
#endif

#endif /* _DELAY_H_ */
