#include "cmsis_os2.h"
#include "rtx_os.h"
#include "hal_system.h"
#include "console.h"
#include "delay.h"

// Hardware assisted microsecond level delays.
// See: https://community.st.com/s/question/0D50X00009XkeRYSAZ/delay-in-us

// Initialize hardware delay resources.
void delay_init(void)
{
  // Enable TRC.
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

#if defined(STM32F7)
  // Unlock software access to the DWT register.
  // This may not be needed on non-STM32F7 devices.
  DWT->LAR = 0xC5ACCE55;
#endif

  // Enable clock cycle counter.
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  // Reset the clock cycle counter value.
  DWT->CYCCNT = 0u;

#if 0
  // 3 NO OPERATION instructions.
  __ASM volatile ("NOP");
  __ASM volatile ("NOP");
  __ASM volatile ("NOP");

  // Check if the clock cycle counter has started.
  if (!DWT->CYCCNT)
  {
    // Report failure on hardware delay counter.
    console_puts("DELAY: failed to initialize hardware delay counter");
  }
#endif
}

