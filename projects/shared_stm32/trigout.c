#include <stdlib.h>
#include "cmsis_os2.h"
#include "rtx_lib.h"
#include "hal_system.h"
#include "clocks.h"
#include "syslog.h"
#include "trigout.h"
#include "shell.h"

// TRIG OUTPUT
// TRIG     PD11

// Trigger pulse timer.
static osTimerId_t trigout_timer_id = NULL;

// Callback for the timer.
static void trigout_timer_callback(void *arg)
{
  (void) arg;

  // Cancel the triger output.
  trigout_set_output(false);
}

// Trigger output shell functionality.
static bool trigout_shell_command(int argc, char **argv)
{
  bool needs_help = false;

  // Parse the command.
  if (argc > 1)
  {
    // Parse the argument.
    if (!strcasecmp(argv[1], "set"))
    {
      // Set the trigger.
      if (argc > 2)
      {
        // Get the value to be set.
        bool value = strtol(argv[2], NULL, 0) != 0;

        // Set the trigger output value.
        trigout_set_output(value);
      }

      shell_printf("Trigger: %u\n", trigout_get_output() ? 1 : 0);
    }
    else if (!strcasecmp(argv[1], "pulse"))
    {
      uint32_t delay = 1000;

      // Set the trigger.
      if (argc > 2) delay = (uint32_t) strtol(argv[2], NULL, 0);

      // Pulse the trigger output.
      trigout_pulse(delay);

      shell_puts("Trigger pulse\n");
    }
    else
    {
      // We need help.
      needs_help = true;
    }
  }
  else
  {
    // We need help.
    needs_help = true;
  }

  // Print help.
  if (needs_help)
  {
    shell_puts("Usage:\n");
    shell_puts("    %s set [0|1]\n");
  }

  return true;
}

// Initialize the trigger output.
void trigout_init(void)
{
  // Enable GPIO and TIM4 peripheral clocks.
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);

  // Configure quadrature output pins.
  LL_GPIO_InitTypeDef gpio_init;
  LL_GPIO_StructInit(&gpio_init);
  gpio_init.Pin = LL_GPIO_PIN_11;
  gpio_init.Mode = LL_GPIO_MODE_OUTPUT;
  // gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  gpio_init.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
  gpio_init.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOD, &gpio_init);

  // Set the pin output.
  LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_11);

  // Static pulse timer initialization.
  static uint32_t trigout_timer_cb[osRtxTimerCbSize/4U] __attribute__((section(".bss.os.timer.cb")));

  // Timer attributes.
  osTimerAttr_t timer_attrs = { .name = "trigout", .attr_bits = 0U,
                                .cb_mem = trigout_timer_cb,
                                .cb_size = sizeof(trigout_timer_cb) };

  // Create the timer.
  trigout_timer_id = osTimerNew(trigout_timer_callback, osTimerOnce, NULL, &timer_attrs);
  if (trigout_timer_id == NULL)
  {
    syslog_printf(SYSLOG_ERROR, "TRIGOUT: cannot create timer");
  }

  // Initialize the shell command.
  shell_add_command("trigout", trigout_shell_command);
}

// Get the current trigger ouput value.
bool trigout_get_output(void)
{
  // Return true if the trigger is set.
  return LL_GPIO_IsOutputPinSet(GPIOD, LL_GPIO_PIN_11) ? false : true;
}

// Set the current trigger output value.
void trigout_set_output(bool value)
{
  // Set or reset the pin output.
  if (value)
    LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_11);
  else
    LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_11);
}

// Pulse the trigger output for the indicated duration.
void trigout_pulse(uint32_t duration_ms)
{
  // Set the triger output.
  trigout_set_output(true);

  // Start the timer for the specified duration.
  osTimerStart(trigout_timer_id, duration_ms);
}

