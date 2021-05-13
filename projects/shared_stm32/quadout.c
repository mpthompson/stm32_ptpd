#include <stdlib.h>
#include "cmsis_os2.h"
#include "hal_system.h"
#include "clocks.h"
#include "quadout.h"
#include "shell.h"

// TIM4
// CH1     PD12
// CH2     PD13

// See: http://www.micromouseonline.com/2016/02/05/clock-pulses-with-variable-phase-stm32/

static uint32_t base_frequency = 840000000;
static uint32_t timer_count = 1;
static uint32_t timer_frequency = 1200;
static uint32_t timer_prescaler = 1;

// Computes preload to match frequency.
static uint32_t quad_prescaler(uint32_t base_freq, uint32_t freq, uint32_t count)
{
  // Determine the preload count to match the desired frequency.
  // 840000000 / (2 * 300) / 1000 = 140
  uint32_t prescaler = base_freq / (2 * freq) / count;

  return prescaler;
}

// Quadrature output shell functionality.
static bool quadout_shell_command(int argc, char **argv)
{
  bool needs_help = false;

  // Parse the command.
  if (argc > 1)
  {
    // Parse the argument.
    if (!strcasecmp(argv[1], "freq"))
    {
      uint32_t freq;

      // Are we passed the frequency?
      if (argc > 2)
      {
        // Parse the frequency.
        freq = strtol(argv[2], NULL, 0);

        // Set the frequency.
        quadout_set_frequency(freq);
      }

      // Get the frequency.
      freq = quadout_get_frequency();

      // Report the frequency.
      shell_printf("frequency: %u hz\n", freq);
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
    shell_printf("    %s freq [frequency]\n", argv[0]);
  }

  return true;
}

void quadout_init(void)
{
  // Enable GPIO and TIM4 peripheral clocks.
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);

  // Configure quadrature output pins.
  LL_GPIO_InitTypeDef gpio_init;
  LL_GPIO_StructInit(&gpio_init);
  gpio_init.Pin = LL_GPIO_PIN_12 | LL_GPIO_PIN_13;
  gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
  // XXX gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  gpio_init.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
  gpio_init.Pull = LL_GPIO_PULL_NO;
  gpio_init.Alternate = LL_GPIO_AF_2;
  LL_GPIO_Init(GPIOD, &gpio_init);

  // Get the base frequency of TIM4.
  base_frequency = clocks_timer_base_frequency(TIM4);

  // Set the count and preload values.
  timer_count = 1000;
  timer_frequency = 1200;
  timer_prescaler = quad_prescaler(base_frequency, timer_frequency, timer_count);

  // Configure the hardware timer with a 600 Hz cycle time.
  // TIM_Prescaler = N - 1; Divides the Bus/TIM clock down by N
  // TIM_Period = N - 1; Divides clock down by N, ie the *period* is N ticks.

  LL_TIM_InitTypeDef tbase_init;
  LL_TIM_StructInit(&tbase_init);
  tbase_init.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  tbase_init.Prescaler = timer_prescaler - 1;
  tbase_init.Autoreload = timer_count - 1;
  LL_TIM_Init(TIM4, &tbase_init);

  // Configure TIM4 channels 1 thru 2 for output.
  LL_TIM_OC_InitTypeDef oc_init;
  LL_TIM_OC_StructInit(&oc_init);
  oc_init.OCMode = LL_TIM_OCMODE_TOGGLE;
  oc_init.CompareValue = 0;
  oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
  oc_init.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
  oc_init.CompareValue = 250;
  LL_TIM_OC_Init(TIM4, LL_TIM_CHANNEL_CH1, &oc_init);
  oc_init.CompareValue = 750;
  LL_TIM_OC_Init(TIM4, LL_TIM_CHANNEL_CH2, &oc_init);

  // Enable TIM4.
  LL_TIM_EnableCounter(TIM4);

  // Initialize the shell command.
  shell_add_command("quadout", quadout_shell_command);
}

// Set the timer frequency.
void quadout_set_frequency(uint32_t freq)
{
  // Reject absurd values.
  if ((freq < 100) || (freq > 4000)) return;

  // Update the timer frequency.
  timer_frequency = freq;

  // Calculate a new prescaler.
  timer_prescaler = quad_prescaler(base_frequency, timer_frequency, timer_count);

  // Set the prescaler.
  LL_TIM_SetPrescaler(TIM4, timer_prescaler);
}

// Get the timer frequency.
uint32_t quadout_get_frequency(void)
{
  return timer_frequency;
}
