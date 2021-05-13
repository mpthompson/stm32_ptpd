#include <math.h>
#include <stdlib.h>
#include "cmsis_os2.h"
#include "hal_system.h"
#include "ntime.h"
#include "clocks.h"

// Hardware time module.  Implements a 64-bit hardware timer independent
// of the disciplined system timer for near microsecond level timing of events.

// Incremented every 1/10th second by the hardware timer.
static volatile uint32_t ntime_counter = 0;

// External timer interrupt.
void TIM8_BRK_TIM12_IRQHandler(void)
{
  // Is the timer interrupt active?
  if ((LL_TIM_IsActiveFlag_UPDATE(TIM12) && LL_TIM_IsEnabledIT_UPDATE(TIM12)) != RESET)
  {
    // Increment the hardware timer count.
    ++ntime_counter;

    // Clear the timer interrupt.
    LL_TIM_ClearFlag_UPDATE(TIM12);
  }
}

// Initialize microsecond timer resources.
void ntime_init(uint32_t timer_priority)
{
  LL_TIM_InitTypeDef time_base_init;

  // Enable hardware timer peripheral clocks.
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM12);

  // Stop the TIM12 timer.
  LL_TIM_DisableCounter(TIM12);

  // Reset the counter.
  ntime_counter = 0;

  // Enable the TIM12 global interrupt in the NVIC.
  uint32_t grouping = NVIC_GetPriorityGrouping();
  uint32_t priority = NVIC_EncodePriority(grouping, timer_priority, 0);
  NVIC_SetPriority(TIM8_BRK_TIM12_IRQn, priority);
  NVIC_EnableIRQ(TIM8_BRK_TIM12_IRQn);

  // Get the base frequency of the timer.
  uint32_t base_frequency = clocks_timer_base_frequency(TIM12);

  // Configure the hardware timer for a 10 Hz interrupt frequency.
  LL_TIM_StructInit(&time_base_init);
  time_base_init.Prescaler = ((base_frequency / 100000) - 1);  // 899
  time_base_init.Autoreload = (10000 - 1); // 9999
  LL_TIM_Init(TIM12, &time_base_init);

  // Reset the counter to start from zero.
  LL_TIM_SetCounter(TIM12, 0);

  // Enable TIM12 timer interrupt.
  LL_TIM_EnableIT_UPDATE(TIM12);

  // Start the TIM12 timer.
  LL_TIM_EnableCounter(TIM12);
}

// Suspect the microsecond hardware timer.
void ntime_suspend(void)
{
  // Stop the TIM12 timer.
  LL_TIM_DisableCounter(TIM12);
}

// Resume the microsecond hardware timer.
void ntime_resume(void)
{
  // Start the TIM12 timer.
  LL_TIM_EnableCounter(TIM12);
}

// Get the current nanosecond timer timestamp.
int64_t ntime_get_nsecs(void)
{
  // Get the counter which forms the high portion of the timestamp.
  uint32_t timestamp_hi = ntime_counter;

  // Get the hardware timer which forms the low portion of the timestamp.
  uint32_t timestamp_lo = LL_TIM_GetCounter(TIM12);

  // Get again the counter which forms the high portion of the timestamp.
  uint32_t timestamp_hi_after = ntime_counter;

  // Has the high portion of the timestamp changed?
  if (timestamp_hi != timestamp_hi_after)
  {
    // If they are not equal, we pulled the time just when a rollover occurred.
    // We need to determine if the low portion of the timestamp goes with the
    // older high portion of the timestamp or the newer high portion.
    if (timestamp_lo < 5000) timestamp_hi = timestamp_hi_after;
  }

  // Create the 64-bit timestamp high and low hardware timer components.
  int64_t timestamp = (((int64_t) timestamp_hi) * 100000000) + ((int64_t) timestamp_lo * 10000);

  return timestamp;
}

// Get the current millisecond timer timestamp.
uint32_t ntime_get_msecs(void)
{
  // Get the microsecond timestamp.
  uint64_t usecs = (uint64_t) ntime_get_nsecs();

  // Convert to an unsigned 32bit millisecond timestamp.
  uint32_t msecs = (uint32_t) (usecs / 1000000);

  return msecs;
}

// Convert a timestamp value to constiuent second and nanosecond parts.
void ntime_to_secs(int64_t timestamp, int32_t *seconds, int32_t *nanoseconds)
{
  // Break into two pieces -- seconds and nanoseconds.
  *seconds = abs((int32_t) (timestamp / 1000000000));
  *nanoseconds = abs((int32_t) (timestamp % 1000000000));
}
