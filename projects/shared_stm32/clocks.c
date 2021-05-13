#include "cmsis_os2.h"
#include "hal_system.h"
#include "tick.h"
#include "shell.h"
#include "clocks.h"

// Print clock information.
static bool clocks_shell_clocks(int argc, char **argv)
{
  LL_RCC_ClocksTypeDef clocks;

  // Get the STM32 clock frequencies.
  LL_RCC_GetSystemClocksFreq(&clocks);

  shell_printf("SYSCLK: %u\n", clocks.SYSCLK_Frequency);
  shell_printf("HCLK: %u\n", clocks.HCLK_Frequency);
  shell_printf("PCLK1: %u\n", clocks.PCLK1_Frequency);
  shell_printf("PCLK2: %u\n", clocks.PCLK2_Frequency);
  shell_printf("TICK: %u\n", tick_get_frequency());

  return true;
}

// Initialize clocks processing.
void clocks_init(void)
{
  // Insert the shell clocks command.
  shell_add_command("clocks", clocks_shell_clocks);
}

// Returns the base frequency of the indicated timer.
uint32_t clocks_timer_base_frequency(TIM_TypeDef *tim)
{
  uint32_t tim_freq;
  uint32_t sys_freq;
  LL_RCC_ClocksTypeDef rcc_clocks;

  // Get the timer frequencies.
  LL_RCC_GetSystemClocksFreq(&rcc_clocks);

  // Get the system frequency.
  sys_freq = rcc_clocks.SYSCLK_Frequency;
  
  // Which peripheral clock is the timer associated with.
  if (((tim) == TIM2) || ((tim) == TIM3) ||
      ((tim) == TIM4) || ((tim) == TIM5) ||
      ((tim) == TIM6) || ((tim) == TIM7) ||
      ((tim) == TIM12) || ((tim) == TIM13) ||
      ((tim) == TIM14))
  {
    // Based on peripheral PCLK1 frequency.
    tim_freq = rcc_clocks.PCLK1_Frequency;
  }
  else if (((tim) == TIM1) || ((tim) == TIM8) ||
           ((tim) == TIM9) || ((tim) == TIM10) ||
           ((tim) == TIM11))
  {
    // Based on peripheral PCLK2 frequency.
    tim_freq = rcc_clocks.PCLK2_Frequency;
  }
  else
  {
    // Invalid timer.
    return 0;
  }

  // Do we need to apply the frequency multiplier?
  if (tim_freq != sys_freq)
  {
    // Yes. Multiply the frequency by two.
    tim_freq *= 2;
  }

  return tim_freq;
}

// Returns the base frequency of the indicated CAN device.
uint32_t clocks_can_base_frequency(CAN_TypeDef *can)
{
  uint32_t can_freq = 0;
  LL_RCC_ClocksTypeDef clocks;

  // Get the clock frequencies.
  LL_RCC_GetSystemClocksFreq(&clocks);

  // Determine the CAN base frequency.
  if ((can) == CAN1)
  {
    can_freq = clocks.PCLK1_Frequency;
  }
#if defined(CAN2)
  else if ((can) == CAN2)
  {
    can_freq = clocks.PCLK1_Frequency;
  }
#endif
#if defined(CAN3)
  else if ((can) == CAN3)
  {
    can_freq = clocks.PCLK1_Frequency;
  }
#endif

  return can_freq;
}

// Enable/disable the clock for the specified GPIO port.
void clocks_gpio_enable(GPIO_TypeDef *port, bool enable)
{
  // Enable the port clock.
  switch ((uint32_t) port)
  {
    case GPIOA_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
      break;
    case GPIOB_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
      break;
    case GPIOC_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
      break;
    case GPIOD_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
      break;
    case GPIOE_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOE);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOE);
      break;
    case GPIOF_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOF);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOF);
      break;
    case GPIOG_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOG);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOG);
      break;
    case GPIOH_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOH);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOH);
      break;
#if defined(GPIOI)
    case GPIOI_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOI);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOI);
      break;
#endif
#if defined(GPIOJ)
    case GPIOJ_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOJ);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOJ);
      break;
#endif
#if defined(GPIOK)
    case GPIOK_BASE:
      if (enable)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOK);
      else
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOK);
      break;
#endif
    default:
      // Enable no clock.
      break;
  }
}
