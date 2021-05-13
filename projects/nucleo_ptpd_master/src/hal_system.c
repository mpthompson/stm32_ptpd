#include "cmsis_os2.h"
#include "hal_system.h"
#include "delay.h"
#include "ntime.h"
#include "ethernetif.h"

// HAL common initialization called from HAL_Init().
void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  // This is configured in HAL_Init(), but we call it again here.
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  // System interrupt initialization.

  // MemoryManagement_IRQn interrupt configuration.
  HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
  // BusFault_IRQn interrupt configuration.
  HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
  // UsageFault_IRQn interrupt configuration.
  HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
  // DebugMonitor_IRQn interrupt configuration.
  HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);

  // RTOS interrupt initialization.

  // SysTick and PendSV need to be the same priority, at least as far as
  // not being able to preempt each other. SVCall needs to be able to
  // preempt the SysTick and PendSV. As a rule of thumb SVC, PendSV and
  // SysTick interrupts are tied to the RTOS and must never be touched
  // by anyone else.

  // SVCall_IRQn interrupt configuration.
  HAL_NVIC_SetPriority(SVCall_IRQn, 14, 0);
  // PendSV_IRQn interrupt configuration.
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
  // SysTick_IRQn interrupt configuration.
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

// System clock configuration. Normally the contents of this
// function is controlled by CubeMX utility.
HAL_StatusTypeDef HAL_SystemClockConfig(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  // Configure the main internal regulator output voltage.
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  // Initializes the CPU, AHB and APB bus clocks.
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    // Something is wrong, return error.
    return HAL_ERROR;
  }

  // Initializes the CPU, AHB and APB bus clocks.
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    // Something is wrong, return error.
    return HAL_ERROR;
  }

  return HAL_OK;
}

// Configures TIM12 as a time base source with a 1ms time base and a dedicated tick
// interrupt priority. This function is called automatically at the beginning of
// program after reset by HAL_Init() or at any time when clock is configured, by
// HAL_RCC_ClockConfig().
//
// Overrides weak function of same name in stm32f4xx_hal.c.
HAL_StatusTypeDef HAL_InitTick(uint32_t tick_priority)
{
  // Initialize the microsecond timer.
  ntime_init(tick_priority);

  return HAL_OK;
}

// Disable the tick increment by disabling TIM14 update interrupt.
//
// Overrides weak function of same name in stm32f4xx_hal.c.
void HAL_SuspendTick(void)
{
  // Suspend the microsecond timer.
  ntime_suspend();
}

// Enable the tick increment by enabling TIM14 update interrupt.
//
// Overrides weak function of same name in stm32f4xx_hal.c.
void HAL_ResumeTick(void)
{
  // Resume the microsecond timer.
  ntime_resume();
}

// This function is called to increment a global variable used as 
// application time base. This does nothing in this implementation.
//
// Overrides weak function of same name in stm32f4xx_hal.c.
void HAL_IncTick(void)
{
  // Does nothing.
}

// Provides a tick value in millisecond.
//
// Overrides weak function of same name in stm32f4xx_hal.c.
uint32_t HAL_GetTick(void)
{
  return ntime_get_msecs();
}

// This function provides minimum delay (in milliseconds).
//
// Overrides weak function of same name in stm32f4xx_hal.c.
void HAL_Delay(uint32_t delay)
{
  // Are we in a thread?
  if (osThreadGetId() != NULL)
  {
    // Yes. Use the RTOS delay function.
    delay_ms(delay);
  }
  else
  {
    // No. Use polling.
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = delay;

    // Add a freq to guarantee minimum wait.
    if (wait < HAL_MAX_DELAY)
    {
      wait += (uint32_t) HAL_GetTickFreq();
    }

    // Wait for duration to elapse.
    while ((HAL_GetTick() - tickstart) < wait)
    {
    }
  }
}

// This function provides minimum delay (in microseconds).
// This is needed because on STM32F4/F7 devices successive write operations to the same
// register might not be fully taken into account. This happens if a previous write to
// the same register is performed within a time period of four TX_CLK/RX_CLK clock cycles.
// When this error occurs, reading the register returns the most recently written value,
// but the Ethernet MAC continues to operate as if the latest write operation never
// occurred.
//
// Overrides weak function of same name in stm32f4xx_hal_eth.c.
void HAL_EthDelay(uint32_t delay)
{
  // The Ethernet delay is specified in microseconds.
  delay_us(delay);
}

// HAL system initialization function. This should be the first
// function called in main().
HAL_StatusTypeDef HAL_SystemInit(void)
{
#if defined(DEBUG)
  // Disable IWDG timers if core is halted.
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM2_STOP;
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM3_STOP;
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM4_STOP;
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM5_STOP;
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM6_STOP;
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM7_STOP;
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM12_STOP;
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM13_STOP;
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM14_STOP;
  DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM1_STOP;
  DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM8_STOP;
  DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM9_STOP;
  DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM10_STOP;
  DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM11_STOP;
#endif

  // Configure instruction and data caches, NVIC group priority, HAL timer tick
  // and low level peripheral initialization.
  if (HAL_Init() != HAL_OK)
  {
    return HAL_ERROR;
  }

  // Configure the system clocks.
  if (HAL_SystemClockConfig() != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

// Ethernet peripheral initalization called from the HAL.
void HAL_ETH_MspInit(ETH_HandleTypeDef *eth_handle)
{
  if (eth_handle->Instance == ETH)
  {
    // Enable Ethernet peripheral clock.
    __HAL_RCC_ETH_CLK_ENABLE();

    // STM32 Nucleo 144 Ethernet GPIO pin configuration
    //
    // ETH_RMII_REF_CLK-------> PA1
    // ETH_MDIO --------------> PA2
    // ETH_RMII_CRS_DV -------> PA7
    // ETH_RMII_TXD1   -------> PB13
    // ETH_MDC ---------------> PC1
    // ETH_RMII_RXD0   -------> PC4
    // ETH_RMII_RXD1   -------> PC5
    // ETH_RMII_TX_EN  -------> PG11
    // ETH_RMII_TXD0   -------> PG13
    //
    // ETH_RST_PIN     -------> PG0 (Modified Nucleo Hardware)

    // Enable GPIO clocks.
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;

    // Configure PA1, PA2 and PA7.
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure PB13.
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Configure PC1, PC4 and PC5.
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Configure PG11 and PG13.
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    // Configure the PHY RST pin.
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    // Reset the Ethernet PHY.
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, GPIO_PIN_SET);
    delay_ms(10);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, GPIO_PIN_RESET);
    delay_ms(10);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, GPIO_PIN_SET);
    delay_ms(10);

    // Peripheral interrupt init.
    HAL_NVIC_SetPriority(ETH_IRQn, ethernetif_config_preempt_priority(), 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
  }
}

// Ethernet peripheral de-initalization called from the HAL.
void HAL_ETH_MspDeInit(ETH_HandleTypeDef *eth_handle)
{
  if (eth_handle->Instance == ETH)
  {
    // Peripheral interrupt deinit.
    HAL_NVIC_DisableIRQ(ETH_IRQn);

    // Peripheral clock disable.
    __HAL_RCC_ETH_CLK_DISABLE();

    // STM32 Nucleo 144 Ethernet GPIO pin configuration
    //
    // ETH_RMII_REF_CLK-------> PA1
    // ETH_MDIO --------------> PA2
    // ETH_RMII_CRS_DV -------> PA7
    // ETH_RMII_TXD1   -------> PB13
    // ETH_MDC ---------------> PC1
    // ETH_RMII_RXD0   -------> PC4
    // ETH_RMII_RXD1   -------> PC5
    // ETH_RMII_TX_EN  -------> PG11
    // ETH_RMII_TXD0   -------> PG13
    //
    // ETH_RST_PIN     -------> PG0 (Modified Nucleo Hardware)

    // Deinit the GPIO pins.
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5);
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_0 | GPIO_PIN_11 | GPIO_PIN_13);
  }
}

