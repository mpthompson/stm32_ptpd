#include <stdint.h>
#include "cmsis_os2.h"
#include "rtx_os.h"
#include "hal_system.h"
#include "event.h"
#include "leds.h"
#include "tick.h"
#include "blink.h"

#if defined(TEST_WATCHDOG)
#include "buttons.h"
#include "watchdog.h"
#endif

// Blink timer id.
static osTimerId_t blink_timer_id = NULL;

// Blink delay.
static uint32_t blink_delay = 50;

// A blink event occured.
static void blink_event(void *arg)
{
  UNUSED(arg);

  // Toggle the blink LED.
  leds_toggle_value(blink_config_led());

#if defined(TEST_WATCHDOG)
  // Is the user button not pressed?
  if (!buttons_get_value(BUTTON_1))
  {
    // Yes. Kick the watchdog.
    watchdog_kick();
  }
#endif
}

// LED blink timer callback handler.  This is called from the
// context of the timer thread.
static void blink_timer_handler(void *arg)
{
  // Schedule a heart beat event.
  event_schedule(blink_event, NULL, 0);
}

// Initialize the blink timer.
void blink_init(void)
{
  // Static timer control blocks.
  static uint32_t blink_timer_cb[osRtxTimerCbSize/4U] __attribute__((section(".bss.os.timer.cb")));

  // Blink timer attributes.
  osTimerAttr_t blink_timer_attrs =
  {
    .name = "blink",
    .attr_bits = 0U,
    .cb_mem = blink_timer_cb,
    .cb_size = sizeof(blink_timer_cb)
  };

  // Create the blink timer.
  blink_timer_id = osTimerNew(blink_timer_handler, osTimerPeriodic, NULL, &blink_timer_attrs);
  if (blink_timer_id)
  {
    // Get the configured blink rate.
    uint32_t blinks_per_second = blink_config_rate();

    // Are we setting a rate?
    if (blinks_per_second)
    {
      // Determine the blink delay from the blink rate.
      blink_delay = 1000 / (blinks_per_second * 2);

      // Start the blink timer running the default delay.
      osTimerStart(blink_timer_id, tick_from_milliseconds(blink_delay));
    }
  }
}

// Set the LED blink rate.
void blink_set_rate(uint32_t blinks_per_second)
{
  // Make sure the timer is initialized.
  if (blink_timer_id)
  {
    // Sanity check the rate.
    if (blinks_per_second > 20) blinks_per_second = 20;

    // Stop the blink timer.
    osTimerStop(blink_timer_id);

    // Are we setting a rate?
    if (blinks_per_second)
    {
      // Determine the blink delay from the blink rate.
      blink_delay = 1000 / (blinks_per_second * 2);

      // Restart the blink timer.
      osTimerStart(blink_timer_id, tick_from_milliseconds(blink_delay));
    }
  }
}

// System configurable blink LED by default is the first LED.
__WEAK uint32_t blink_config_led(void)
{
  return 0;
}

// System configurable initial blink rate.
__WEAK uint32_t blink_config_rate(void)
{
  return 2u;
}


