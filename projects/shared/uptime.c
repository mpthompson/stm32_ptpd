#include "cmsis_os2.h"
#include "tick.h"
#include "uptime.h"

// Return the uptime in milliseconds.
uint32_t uptime_get(void)
{
  // Convert RTOS ticks to milliseconds.
  return tick_to_milliseconds(osKernelGetTickCount());
}
