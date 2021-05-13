#include <stdint.h>
#include "cmsis_os2.h"
#include "hal_system.h"
#include "delay.h"
#include "reboot.h"

// Reboot the system without delay.
void reboot_system_no_delay(void)
{
  // Reboot system via core peripherals function call.
  NVIC_SystemReset();
}

// Reboot the system.
void reboot_system(void)
{
  // If in a thread, sleep a bit to allow other threads to cleanup.
  if (osThreadGetId() != NULL) delay_ms(250);

  // Reboot system now.
  reboot_system_no_delay();
}

// Reboot event handler.
void reboot_event(void *arg)
{
  UNUSED(arg);

  // Reboot the system.
  reboot_system();
}
