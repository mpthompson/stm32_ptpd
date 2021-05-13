#include "cmsis_os2.h"
#include "hal_system.h"
#include "lwip/opt.h"
#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#include "lwip/netifapi.h"
#include "lwip/apps/mdns.h"
#include "delay.h"
#include "event.h"
#include "tick.h"
#include "clocks.h"
#include "buttons.h"
#include "leds.h"
#include "peek.h"
#include "blink.h"
#include "extint.h"
#include "random.h"
#include "console.h"
#include "network.h"
#include "shell.h"
#include "telnet.h"
#include "syslog.h"
#include "systime.h"
#include "hardtime.h"
#include "watchdog.h"
#include "gps.h"

// Initialization function type.
typedef void (*init_func_t)(void);

// Array of initialization functions.
static const init_func_t init_functions[] =
{
  shell_init,
  console_shell_init,
  tick_init,
  clocks_init,
  buttons_init,
  leds_init,
  blink_init,
  network_init,
  hardtime_init,
  systime_init,
  syslog_init,
  telnet_init,
  peek_init,
  gps_init,
  NULL
};

// Perform system initialization via a series of event callbacks. This mechanism
// allows system initialization to be interleaved with event processing for modules
// that were previously initialized.
static void init_process(void *arg)
{
  UNUSED(arg);

  // Index init initialization step.
  static uint32_t init_step = 0;

  // Get the next initialization function.
  init_func_t init_function = init_functions[init_step];

  // Do we have an initialization function?
  if (init_function != NULL)
  {
    // Call the initialization function.
    init_function();

    // Increment the initialization step.
    init_step += 1;

    // Schedule this function to be called again.
    event_schedule(init_process, NULL, osWaitForever);
  }
  else
  {
    // Initialization is complete. This function
    // will not be called again.

    // Configure a lower syslog level.
    syslog_set_level(SYSLOG_INFO);
  }
}

// Main application thread.
static void main_thread(void *argument)
{
  // Initialize event processing module.
  event_init();

  // Schedule initialization.
  event_schedule(init_process, NULL, osWaitForever);

  // Process events.
  event_loop();
}

// Main application entry point.
int main(void)
{
  // Initialize the STM32 HAL (hardware abstraction layer) system.
  HAL_SystemInit();

  // Initialize external interrupt handling.
  extint_init();

  // Initialize the debug console.
  console_init();

  // Initialize delay hardware.
  delay_init();

  // Initialize random number hardware.
  random_init();

  // Display console message indicating kernel initialization.
  console_puts("\nInitializing RTOS kernel...\n");

  // Initialize CMSIS-RTOS.
  if (osKernelInitialize() != osOK)
  {
    console_puts("ERROR: Failed to initialize kernel\n");
  }

  // Create the main application thread.
  if (osThreadNew(main_thread, NULL, NULL) == NULL)
  {
    console_puts("ERROR: Failed to create main thread\n");
  }

  // Is the kernel ready to start?
  if (osKernelGetState() != osKernelReady)
  {
    console_puts("ERROR: Kernel state is invalid for starting\n");
  }

  // Start the kernel. Should never return from this function call.
  if (osKernelStart() != osOK)
  {
    console_puts("ERROR: Failure starting kernel\n");
  }
  else
  {
    console_puts("ERROR: Failed to start kernel\n");
  }

  // Should never get here.
  for (;;);
}

#ifdef USE_FULL_ASSERT
// Reports the name of the source file and the source line number
// where the assert_param error has occurred.
void assert_failed(uint8_t* file, uint32_t line)
{
  // User can add his own implementation to report the file name and line number,
  // ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  for (;;);
}
#endif

//
// System configurable overrides.
//

// Global for shutdown of system.
void shutdown_system(void)
{
  // Does nothing on this architecture.
  for (;;);
}

// System configurable DHCP flag.
bool network_config_use_dhcp(void)
{
  return false;
}

// System configurable network IP address.
ip4_addr_t network_config_address(void)
{
  return network_str_to_address("192.168.1.76");
}

// System configurable netmask.
ip4_addr_t network_config_netmask(void)
{
  return network_str_to_address("255.255.255.0");
}

// System configurable gateway IP address.
ip4_addr_t network_config_gateway(void)
{
  return network_str_to_address("192.168.1.1");
}

// System configurable hardware address.
hwaddr_t network_config_hwaddr(void)
{
  hwaddr_t hwaddr = { 6, { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x4C } };
  return hwaddr;
}

// System configurable hostname.
const char *network_config_hostname(void)
{
  return "PTPDMASTER";
}

#if LWIP_PTPD
// System configurable PTPD flag.
bool network_config_ptpd_slave_only(void)
{
  return false;
}
#endif
