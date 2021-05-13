#ifndef _TICK_H
#define _TICK_H

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "cmsis_compiler.h"
#include "os_tick.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global indicating the tick frequency.
extern uint32_t tick_frequency;

void tick_init(void);
uint32_t tick_get_percent_idle(void);

// Return the tick frequency.
__STATIC_INLINE uint32_t tick_get_frequency(void)
{
  return tick_frequency;
}

// Convert milliseconds to tick count.
__STATIC_INLINE uint32_t tick_from_milliseconds(uint32_t milliseconds)
{
  uint32_t ticks = milliseconds;
  // Preserve 0 and osWaitForever.
  if ((milliseconds > 0) && (milliseconds < 0xFFFFFFFFU))
  {
    // Convert milliseconds to RTOS tick count with a minimum of one tick.
    ticks = (uint32_t) (((((uint64_t) milliseconds) * tick_frequency) + 500U) / 1000U);
    if (ticks < 1) ticks = 1;
  }
  return ticks;
}

// Convert microseconds to tick count.
__STATIC_INLINE uint32_t tick_from_microseconds(uint32_t microseconds)
{
  uint32_t ticks = microseconds;
  // Preserve 0 and osWaitForever.
  if ((microseconds > 0) && (microseconds < 0xFFFFFFFFU))
  {
    // Convert microseconds to RTOS tick count with a minimum of one tick.
    ticks = (uint32_t) (((((uint64_t) microseconds) * tick_frequency) + 500000U) / 1000000U);
    if (ticks < 1) ticks = 1;
  }
  return ticks;
}

// Convert tick count to milliseconds.
__STATIC_INLINE uint32_t tick_to_milliseconds(uint32_t ticks)
{
  uint32_t milliseconds = ticks;
  // Preserve 0 and osWaitForever.
  if ((ticks > 0) && (ticks < 0xFFFFFFFFU))
  {
    // Convert RTOS tick count to milliseconds.
    milliseconds = (uint32_t) (((((uint64_t) ticks) * 1000U) + 500U) / tick_frequency);
  }
  return milliseconds;
}

// Convert tick count microseconds.
__STATIC_INLINE uint32_t tick_to_microseconds(uint32_t ticks)
{
  uint32_t microseconds = ticks;
  // Preserve 0 and osWaitForever.
  if ((ticks > 0) && (ticks < 0xFFFFFFFFU))
  {
    // Convert RTOS tick count to microseconds.
    microseconds = (uint32_t) (((((uint64_t) ticks) * 1000000U) + 500000U) / tick_frequency);
  }
  return microseconds;
}

// Return true if the time in milliseconds elapsed.
__STATIC_INLINE bool tick_elapsed(uint32_t start, uint32_t current, uint32_t elapsed_ms)
{
  return (tick_to_milliseconds(current - start) >= elapsed_ms) ? true : false;
}

#ifdef __cplusplus
}
#endif

#endif /* _TICK_H */
