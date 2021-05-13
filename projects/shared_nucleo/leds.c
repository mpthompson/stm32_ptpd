#include <string.h>
#include "hal_system.h"
#include "shell.h"
#include "clocks.h"
#include "leds.h"

// LED Hardware Configuration
//
// STM32F767 Nucleo-144 LED GPIO Outputs
// STM32F429 Nucleo-144 LED GPIO Outputs
//
// PB0      LED_GREEN
// PB7      LED_BLUE
// PB14     LED_RED
//

// GPIOS used to set LEDs on and off.
// Note: Pointer is constant and immutable, but not the pointed data.
static GPIO_TypeDef * const led_output_gpios[LED_COUNT] =
{
  GPIOB,
  GPIOB,
  GPIOB
};

// Pins used to set LEDs on and off.
static const uint16_t led_output_pins[LED_COUNT] = 
{
  LL_GPIO_PIN_0,
  LL_GPIO_PIN_7,
  LL_GPIO_PIN_14
};

// Set the LED colors from the shell.
static bool leds_shell_led(int argc, char **argv)
{
  bool help = false;

  // Should turn an LED on or off?
  if (argc == 3)
  {
    uint32_t led_index = LED_COUNT;
    uint32_t led_mode = 2;

    // Convert the color to an index.
    if (!strcasecmp(argv[1], "green"))
    {
      led_index = LED_GREEN;
    }
    else if (!strcasecmp(argv[1], "red"))
    {
      led_index = LED_RED;
    }
    else if (!strcasecmp(argv[1], "blue"))
    {
      led_index = LED_BLUE;
    }

    // Parse the on/off string.
    if (!strcasecmp(argv[2], "on"))
    {
      led_mode = 1;
    }
    else if (!strcasecmp(argv[2], "off"))
    {
      led_mode = 0;
    }

    // Did we correctly parse the LED index and mode?
    if ((led_index < LED_COUNT) && (led_mode < 2))
    {
      // Set the LED.
      leds_set_value(led_index, led_mode == 1 ? true : false);
    }
    else
    {
      help = true;
    }
  }
  else
  {
    help = true;
  }

  // Do we need to display help?
  if (help)
  {
    shell_puts("Usage:\n");
    shell_puts("  led green|red|blue [on|off]\n");
  }
  
  return true;
}

void leds_init(void)
{
  LL_GPIO_InitTypeDef gpio_init;

  // Loop over each LED GPIO line.
  for (uint32_t i = 0; i < LED_COUNT; ++i)
  {
    // Initialize the LED GPIO peripheral clock.
    clocks_gpio_enable(led_output_gpios[i], true);

    // Initialize LED GPIO to off.
    LL_GPIO_ResetOutputPin(led_output_gpios[i], led_output_pins[i]);

    // Configure LED GPIO for output.
    LL_GPIO_StructInit(&gpio_init);
    gpio_init.Pin = led_output_pins[i];
    gpio_init.Mode = LL_GPIO_MODE_OUTPUT;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_LOW;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = LL_GPIO_PULL_NO;
    gpio_init.Alternate = LL_GPIO_AF_0;
    LL_GPIO_Init(led_output_gpios[i], &gpio_init);
  }

  // Add the shell command.
  shell_add_command("led", leds_shell_led);
}

void leds_set_value(uint32_t led_index, bool led_value)
{
  // Sanity check argument.
  if (led_index >= LED_COUNT) return;

  // Reset or set the LED value.
  if (led_value)
    LL_GPIO_SetOutputPin(led_output_gpios[led_index], led_output_pins[led_index]);
  else
    LL_GPIO_ResetOutputPin(led_output_gpios[led_index], led_output_pins[led_index]);
}

void leds_toggle_value(uint32_t led_index)
{
  // Sanity check argument.
  if (led_index >= LED_COUNT) return;

  // Toggle the LED value.
  if (LL_GPIO_IsOutputPinSet(led_output_gpios[led_index], led_output_pins[led_index]))
    LL_GPIO_ResetOutputPin(led_output_gpios[led_index], led_output_pins[led_index]);
  else
    LL_GPIO_SetOutputPin(led_output_gpios[led_index], led_output_pins[led_index]);
}

