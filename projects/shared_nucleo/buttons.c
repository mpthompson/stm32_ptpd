#include <string.h>
#include "hal_system.h"
#include "shell.h"
#include "buttons.h"

// BUTTON Hardware Configuration
//
// STM32F4XX and STM32F7XX Nucleo-144 GPIO Outputs
//
// PC13     BUTTON_1
//

// GPIOS used to get button on and off.
// Note: Pointer is constant and immutable, but not the pointed data.
static GPIO_TypeDef * const buttons_input_gpios[BUTTON_COUNT] =
{
  GPIOC
};

// Pins used to get button on and off.
static const uint16_t buttons_input_pins[BUTTON_COUNT] = 
{
  LL_GPIO_PIN_13
};

// Get the button values from the shell.
static bool buttons_shell_button(int argc, char **argv)
{
  bool help = false;

  // Display the button value.
  if (argc == 2)
  {
    bool button_state;
    uint32_t button_index = BUTTON_COUNT;

    // Convert the color to an index.
    if (!strcasecmp(argv[1], "button1"))
    {
      button_index = BUTTON_1;
    }

    // Did we correctly parse the button index?
    if (button_index < BUTTON_COUNT)
    {
      // Get the button state.
      button_state = buttons_get_value(button_index);

      // Print the button state.
      shell_printf("Button: %s\n", button_state ? "pressed" : "released");
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
    shell_puts("  button [button1]\n");
  }
  
  return true;
}

void buttons_init(void)
{
  LL_GPIO_InitTypeDef gpio_init;

  // Enable button related peripheral clocks.
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);

  // Configure GPIOC for output.
  LL_GPIO_StructInit(&gpio_init);
  gpio_init.Pin = LL_GPIO_PIN_13;
  gpio_init.Mode = LL_GPIO_MODE_INPUT;
  gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.Pull = LL_GPIO_PULL_NO;
  gpio_init.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOC, &gpio_init);

  // Add the shell command.
  shell_add_command("button", buttons_shell_button);
}

bool buttons_get_value(uint32_t button_index)
{
  // Sanity check argument.
  if (button_index >= BUTTON_COUNT) return false;

  // Return the button value.
  return LL_GPIO_IsInputPinSet(buttons_input_gpios[button_index], buttons_input_pins[button_index]) ? true : false;
}

