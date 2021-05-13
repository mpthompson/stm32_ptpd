#include <stdint.h>
#include "cmsis_os2.h"
#include "hal_system.h"
#include "watchdog.h"

// Initialize the watchdog timer.
void watchdog_init(void)
{
#if defined(DEBUG)
  // Disable IWDG if core is halted.
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
#endif

#if USE_WATCHDOG
  // Check if the system has resumed from IWDG reset.
  if (RCC->CSR & RCC_CSR_IWDGRSTF)
  {
    // Clear reset flags.
    RCC->CSR |= RCC_CSR_RMVF;
  }
#endif
}

// Start the watchdog timer.
void watchdog_start(watchdog_timeout_t expire_time)
{
#if USE_WATCHDOG
  uint16_t reload = 0;

  // Set counter reload value.
  switch (expire_time)
  {
    case WATCHDOG_TIMEOUT_5MS:
      reload = 5; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_10MS:
      reload = 10; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_15MS:
      reload = 15; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_30MS:
      reload = 31; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_60MS:
      reload = 61; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_120MS:
      reload = 123; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_250MS:
      reload = 255; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_500MS:
      reload = 511; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_1S:
      reload = 1023; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_2S:
      reload = 2047; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_4S:
      reload = 4095; // 1024 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_8S:
      reload = 1023; // 128 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_16S:
      reload = 2047; // 128 Hz IWDG ticking.
      break;
    case WATCHDOG_TIMEOUT_32S:
      reload = 4095; // 128 Hz IWDG ticking.
      break;
    default:
      break;
  }

  // Do we have a time?
  if (reload != 0)
  {
    // Enable write access to IWDG_PR and IWDG_RLR registers.
    IWDG->KR = 0x5555;

    // Set proper clock depending on timeout user select.
    if (expire_time >= WATCHDOG_TIMEOUT_8S)
    {
      // IWDG counter clock: LSI/256 = 128Hz.
      IWDG->PR = 0x07;
    }
    else
    {
      // IWDG counter clock: LSI/32 = 1024Hz.
      IWDG->PR = 0x03;
    }

    // Set reload.
    IWDG->RLR = reload;

    // Reload IWDG counter.
    IWDG->KR = 0xAAAA;

    // Enable IWDG (the LSI oscillator will be enabled by hardware).
    IWDG->KR = 0xCCCC;
  }
#endif
}

// Kick the watchdog to prevent system reset.
void watchdog_kick(void)
{
#if USE_WATCHDOG
  // Reload the IWDG counter.
  IWDG->KR = 0xAAAA;
#endif
}
