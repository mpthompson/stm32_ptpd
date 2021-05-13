#include <stdlib.h>
#include "cmsis_os2.h"
#include "hal_system.h"
#include "delay.h"
#include "random.h"

// Configures random number generator.
// WARNING: This function called prior to RTOS initialization.
void random_init(void)
{
  uint32_t seed;

  // Enable random number generator clock source.
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_RNG);

  // Enable random number generator peripheral.
  LL_RNG_Enable(RNG);

  // Wait until random number generator is ready.
  while (!LL_RNG_IsActiveFlag_DRDY(RNG));

  // Read random seed.
  seed = LL_RNG_ReadRandData32(RNG);

  // Seed the standard library random number function.
  srand(seed);
}

// Returns random number.
uint32_t random_get(void)
{
  // Wait until random number generator is ready.
  while (!LL_RNG_IsActiveFlag_DRDY(RNG));

  // Return a 32bit random number.
  return LL_RNG_ReadRandData32(RNG);
}

// Delays a random time between the minimum time and maximum time.
void random_delay(uint32_t min_ms, uint32_t max_ms)
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
  delay_ms(min_ms + (random_get() % (max_ms - min_ms)));
}

