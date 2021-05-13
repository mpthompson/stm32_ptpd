#include <limits.h>
#include "hal_system.h"
#include "ethptp.h"

// WARNING: This modules requires the STM32 Ethernet peripheral be initialized
// from within another module to function. Normally this is done by the module
// that initialize the STM32 Ethernet perhipheral for network communcation.

// Conversion from hardware to PTP format.
static uint32_t subsecond_to_nanosecond(uint32_t subsecond_value)
{
  uint64_t val = subsecond_value * 1000000000ll;
  val >>= 31;
  return val;
}

// Conversion from PTP to hardware format.
static uint32_t nanosecond_to_subsecond(uint32_t subsecond_value)
{
  uint64_t val = subsecond_value * 0x80000000ll;
  val /= 1000000000;
  return val;
}

/***
  **
  ** The functions below were ported from the old STM32 Standard Peripheral Library.
  **
  **/

/**
  * @brief  Updated the PTP block for fine correction with the Time Stamp Addend register value.
  * @param  None
  * @retval None
  */
void ETH_EnablePTPTimeStampAddend(void)
{
  uint32_t tmpreg;

  /* Enable the PTP block update with the Time Stamp Addend register value */
  ETH->PTPTSCR |= ETH_PTPTSCR_TSARU;

  /* Wait until the write operation will be taken into account :
   at least four TX_CLK/RX_CLK clock cycles */
  tmpreg = ETH->PTPTSCR;
  HAL_EthDelay(ETH_REG_WRITE_DELAY);
  ETH->PTPTSCR = tmpreg;
}

/**
  * @brief  Enable the PTP Time Stamp interrupt trigger
  * @param  None
  * @retval None
  */
void ETH_EnablePTPTimeStampInterruptTrigger(void)
{
  uint32_t tmpreg;

  /* Enable the PTP target time interrupt */
  ETH->PTPTSCR |= ETH_PTPTSCR_TSITE;

  /* Wait until the write operation will be taken into account :
   at least four TX_CLK/RX_CLK clock cycles */
  tmpreg = ETH->PTPTSCR;
  HAL_EthDelay(ETH_REG_WRITE_DELAY);
  ETH->PTPTSCR = tmpreg;
}

/**
  * @brief  Updated the PTP system time with the Time Stamp Update register value.
  * @param  None
  * @retval None
  */
void ETH_EnablePTPTimeStampUpdate(void)
{
  uint32_t tmpreg;

  /* Enable the PTP system time update with the Time Stamp Update register value */
  ETH->PTPTSCR |= ETH_PTPTSCR_TSSTU;

  /* Wait until the write operation will be taken into account :
   at least four TX_CLK/RX_CLK clock cycles */
  tmpreg = ETH->PTPTSCR;
  HAL_EthDelay(ETH_REG_WRITE_DELAY);
  ETH->PTPTSCR = tmpreg;
}

/**
  * @brief  Initialize the PTP Time Stamp
  * @param  None
  * @retval None
  */
void ETH_InitializePTPTimeStamp(void)
{
  uint32_t tmpreg;

  /* Initialize the PTP Time Stamp */
  ETH->PTPTSCR |= ETH_PTPTSCR_TSSTI;

  /* Wait until the write operation will be taken into account :
   at least four TX_CLK/RX_CLK clock cycles */
  tmpreg = ETH->PTPTSCR;
  HAL_EthDelay(ETH_REG_WRITE_DELAY);
  ETH->PTPTSCR = tmpreg;
}

/**
  * @brief  Selects the PTP Update method
  * @param  UpdateMethod: the PTP Update method
  *   This parameter can be one of the following values:
  *     @arg ETH_PTP_FineUpdate   : Fine Update method
  *     @arg ETH_PTP_CoarseUpdate : Coarse Update method
  * @retval None
  */
void ETH_PTPUpdateMethodConfig(uint32_t UpdateMethod)
{
  uint32_t tmpreg;

  /* Check the parameters */
  assert_param(IS_ETH_PTP_UPDATE(UpdateMethod));

  if (UpdateMethod != ETH_PTP_CoarseUpdate)
  {
    /* Enable the PTP Fine Update method */
    ETH->PTPTSCR |= ETH_PTPTSCR_TSFCU;
  }
  else
  {
    /* Disable the PTP Coarse Update method */
    ETH->PTPTSCR &= (~(uint32_t)ETH_PTPTSCR_TSFCU);
  }

  /* Wait until the write operation will be taken into account :
   at least four TX_CLK/RX_CLK clock cycles */
  tmpreg = ETH->PTPTSCR;
  HAL_EthDelay(ETH_REG_WRITE_DELAY);
  ETH->PTPTSCR = tmpreg;
}

/**
  * @brief  Enables or disables the PTP time stamp for transmit and receive frames.
  * @param  NewState: new state of the PTP time stamp for transmit and receive frames
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void ETH_PTPTimeStampCmd(FunctionalState NewState)
{
  uint32_t tmpreg;

  /* Check the parameters */
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Enable the PTP time stamp for transmit and receive frames */
    ETH->PTPTSCR |= ETH_PTPTSCR_TSE | ETH_PTPTSSR_TSSIPV4FE | ETH_PTPTSSR_TSSIPV6FE | ETH_PTPTSSR_TSSARFE;
  }
  else
  {
    /* Disable the PTP time stamp for transmit and receive frames */
    ETH->PTPTSCR &= (~(uint32_t)ETH_PTPTSCR_TSE);
  }

  /* Wait until the write operation will be taken into account :
   at least four TX_CLK/RX_CLK clock cycles */
  tmpreg = ETH->PTPTSCR;
  HAL_EthDelay(ETH_REG_WRITE_DELAY);
  ETH->PTPTSCR = tmpreg;
}

/**
  * @brief  Checks whether the specified ETHERNET PTP flag is set or not.
  * @param  ETH_PTP_FLAG: specifies the flag to check.
  *   This parameter can be one of the following values:
  *     @arg ETH_PTP_FLAG_TSARU : Addend Register Update
  *     @arg ETH_PTP_FLAG_TSITE : Time Stamp Interrupt Trigger Enable
  *     @arg ETH_PTP_FLAG_TSSTU : Time Stamp Update
  *     @arg ETH_PTP_FLAG_TSSTI  : Time Stamp Initialize
  * @retval The new state of ETHERNET PTP Flag (SET or RESET).
  */
FlagStatus ETH_GetPTPFlagStatus(uint32_t ETH_PTP_FLAG)
{
  FlagStatus bitstatus = RESET;

  /* Check the parameters */
  assert_param(IS_ETH_PTP_GET_FLAG(ETH_PTP_FLAG));

  if ((ETH->PTPTSCR & ETH_PTP_FLAG) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}

/**
  * @brief  Sets the system time Sub-Second Increment value.
  * @param  SubSecondValue: specifies the PTP Sub-Second Increment Register value.
  * @retval None
  */
void ETH_SetPTPSubSecondIncrement(uint32_t SubSecondValue)
{
  /* Check the parameters */
  assert_param(IS_ETH_PTP_SUBSECOND_INCREMENT(SubSecondValue));

  /* Set the PTP Sub-Second Increment Register */
  ETH->PTPSSIR = SubSecondValue;
}

/**
  * @brief  Sets the Time Stamp update sign and values.
  * @param  Sign: specifies the PTP Time update value sign.
  *   This parameter can be one of the following values:
  *     @arg ETH_PTP_PositiveTime : positive time value.
  *     @arg ETH_PTP_NegativeTime : negative time value.
  * @param  SecondValue: specifies the PTP Time update second value.
  * @param  SubSecondValue: specifies the PTP Time update sub-second value.
  *   This parameter is a 31 bit value, bit32 correspond to the sign.
  * @retval None
  */
void ETH_SetPTPTimeStampUpdate(uint32_t Sign, uint32_t SecondValue, uint32_t SubSecondValue)
{
  /* Check the parameters */
  assert_param(IS_ETH_PTP_TIME_SIGN(Sign));
  assert_param(IS_ETH_PTP_TIME_STAMP_UPDATE_SUBSECOND(SubSecondValue));

  /* Set the PTP Time Update High Register */
  ETH->PTPTSHUR = SecondValue;

  /* Set the PTP Time Update Low Register with sign */
  ETH->PTPTSLUR = Sign | SubSecondValue;
}

/**
  * @brief  Sets the Time Stamp Addend value.
  * @param  Value: specifies the PTP Time Stamp Addend Register value.
  * @retval None
  */
void ETH_SetPTPTimeStampAddend(uint32_t Value)
{
  /* Set the PTP Time Stamp Addend Register */
  ETH->PTPTSAR = Value;
}

/**
  * @brief  Sets the Target Time registers values.
  * @param  HighValue: specifies the PTP Target Time High Register value.
  * @param  LowValue: specifies the PTP Target Time Low Register value.
  * @retval None
  */
void ETH_SetPTPTargetTime(uint32_t HighValue, uint32_t LowValue)
{
  /* Set the PTP Target Time High Register */
  ETH->PTPTTHR = HighValue;
  /* Set the PTP Target Time Low Register */
  ETH->PTPTTLR = LowValue;
}

/**
  * @brief  Get the specified ETHERNET PTP register value.
  * @param  ETH_PTPReg: specifies the ETHERNET PTP register.
  *   This parameter can be one of the following values:
  *     @arg ETH_PTPTSCR  : Sub-Second Increment Register
  *     @arg ETH_PTPSSIR  : Sub-Second Increment Register
  *     @arg ETH_PTPTSHR  : Time Stamp High Register
  *     @arg ETH_PTPTSLR  : Time Stamp Low Register
  *     @arg ETH_PTPTSHUR : Time Stamp High Update Register
  *     @arg ETH_PTPTSLUR : Time Stamp Low Update Register
  *     @arg ETH_PTPTSAR  : Time Stamp Addend Register
  *     @arg ETH_PTPTTHR  : Target Time High Register
  *     @arg ETH_PTPTTLR  : Target Time Low Register
  * @retval The value of ETHERNET PTP Register value.
  */
uint32_t ETH_GetPTPRegister(uint32_t ETH_PTPReg)
{
  /* Check the parameters */
  assert_param(IS_ETH_PTP_REGISTER(ETH_PTPReg));

  /* Return the selected register value */
  return (*(__IO uint32_t *)(ETH_MAC_BASE + ETH_PTPReg));
}

// Examples of subsecond increment and addend values using SysClk = 144 MHz
//
// Addend * Increment = 2^63 / SysClk
//
// ptp_tick = Increment * 10^9 / 2^31
//
// +-----------+-----------+------------+
// | ptp tick  | Increment | Addend     |
// +-----------+-----------+------------+
// |  119 ns   |   255     | 0x0EF8B863 |
// |  100 ns   |   215     | 0x11C1C8D5 |
// |   50 ns   |   107     | 0x23AE0D90 |
// |   20 ns   |    43     | 0x58C8EC2B |
// |   14 ns   |    30     | 0x7F421F4F |
// +-----------+-----------+------------+

#if defined(USE_STM32F4_DISCOVERY)
// Examples of subsecond increment and addend values using SysClk = 168 MHz
//
// Addend * Increment = 2^63 / SysClk
//
// ptp_tick = Increment * 10^9 / 2^31
//
// +-----------+-----------+------------+
// | ptp tick  | Increment | Addend     |
// +-----------+-----------+------------+
// |  119 ns   |   255     | 0x0CD53055 |
// |  100 ns   |   215     | 0x0F386300 |
// |   50 ns   |   107     | 0x1E953032 |
// |   20 ns   |    43     | 0x4C19EF00 |
// |   14 ns   |    30     | 0x6D141AD6 |
// +-----------+-----------+------------+

#define ADJ_FREQ_BASE_ADDEND      0x4C19EF00
#define ADJ_FREQ_BASE_INCREMENT   43
#endif

#if defined(USE_STM32F4XX_NUCLEO_144)
// Examples of subsecond increment and addend values using SysClk = 168 MHz
//
// Addend * Increment = 2^63 / SysClk
//
// ptp_tick = Increment * 10^9 / 2^31
//
// +-----------+-----------+------------+
// | ptp tick  | Increment | Addend     |
// +-----------+-----------+------------+
// |  119 ns   |   255     | 0x0CD53055 |
// |  100 ns   |   215     | 0x0F386300 |
// |   50 ns   |   107     | 0x1E953032 |
// |   20 ns   |    43     | 0x4C19EF00 |
// |   14 ns   |    30     | 0x6D141AD6 |
// +-----------+-----------+------------+

#define ADJ_FREQ_BASE_ADDEND      0x4C19EF00
#define ADJ_FREQ_BASE_INCREMENT   43
#endif

#if 0
// Examples of subsecond increment and addend values using SysClk = 180 MHz
//
// Addend * Increment = 2^63 / SysClk
//
// ptp_tick = Increment * 10^9 / 2^31
//
// +-----------+-----------+------------+
// | ptp tick  | Increment | Addend     |
// +-----------+-----------+------------+
// |  119 ns   |   255     | 0x0BFA2D1D |
// |  100 ns   |   215     | 0x0E34A0AB |
// |   50 ns   |   107     | 0x1C8B3E0D |
// |   20 ns   |    43     | 0x47072356 |
// |   14 ns   |    30     | 0x65CE7F73 |
// +-----------+-----------+------------+

#define ADJ_FREQ_BASE_ADDEND      0x47072356
#define ADJ_FREQ_BASE_INCREMENT   43
#endif

#if defined(USE_STM32F7XX_NUCLEO_144)
// Examples of subsecond increment and addend values using SysClk = 216 MHz
//
// Addend * Increment = 2^63 / SysClk
//
// ptp_tick = Increment * 10^9 / 2^31
//
// +-----------+-----------+------------+
// | ptp tick  | Increment | Addend     |
// +-----------+-----------+------------+
// |  119 ns   |   255     | 0x09FB2598 |
// |  100 ns   |   215     | 0x0BD685E4 |
// |   50 ns   |   107     | 0x17C95E60 |
// |   20 ns   |    43     | 0x3B309D72 |
// |   14 ns   |    30     | 0x54D6BF8A |
// +-----------+-----------+------------+

#define ADJ_FREQ_BASE_ADDEND      0x3B309D72
#define ADJ_FREQ_BASE_INCREMENT   43
#endif

#if defined(USE_STM32F7XX_AMBER) || defined(USE_STM32F7XX_PRINTER)
// Examples of subsecond increment and addend values using SysClk = 216 MHz
//
// Addend * Increment = 2^63 / SysClk
//
// ptp_tick = Increment * 10^9 / 2^31
//
// +-----------+-----------+------------+
// | ptp tick  | Increment | Addend     |
// +-----------+-----------+------------+
// |  119 ns   |   255     | 0x09FB2598 |
// |  100 ns   |   215     | 0x0BD685E4 |
// |   50 ns   |   107     | 0x17C95E60 |
// |   20 ns   |    43     | 0x3B309D72 |
// |   14 ns   |    30     | 0x54D6BF8A |
// +-----------+-----------+------------+

#define ADJ_FREQ_BASE_ADDEND      0x3B309D72
#define ADJ_FREQ_BASE_INCREMENT   43
#endif

// Update method is ETH_PTP_FineUpdate or ETH_PTP_CoarseUpdate.
void ethptp_start(uint32_t update_method)
{
  // Mask the time stamp trigger interrupt by setting bit 9 in the MACIMR register.
  ETH->MACIMR &= ~(ETH_MAC_IT_TST);

  // Program Time stamp register bit 0 to enable time stamping.
  ETH_PTPTimeStampCmd(ENABLE);

  // Program the Subsecond increment register based on the PTP clock frequency.
  ETH_SetPTPSubSecondIncrement(ADJ_FREQ_BASE_INCREMENT); // to achieve 20 ns accuracy, the value is ~ 43

  if (update_method == ETH_PTP_FineUpdate)
  {
    // If you are using the Fine correction method, program the Time stamp addend register
    // and set Time stamp control register bit 5 (addend register update).
    ETH_SetPTPTimeStampAddend(ADJ_FREQ_BASE_ADDEND);
    ETH_EnablePTPTimeStampAddend();

    // Poll the Time stamp control register until bit 5 is cleared.
    while(ETH_GetPTPFlagStatus(ETH_PTP_FLAG_TSARU) == SET);
  }

  // To select the Fine correction method (if required),
  // program Time stamp control register  bit 1.
  ETH_PTPUpdateMethodConfig(update_method);

  // Program the Time stamp high update and Time stamp low update registers
  // with the appropriate time value.
  ETH_SetPTPTimeStampUpdate(ETH_PTP_PositiveTime, 0, 0);

  // Set Time stamp control register bit 2 (Time stamp init).
  ETH_InitializePTPTimeStamp();

  // The enhanced descriptor format is enabled and the descriptor size is
  // increased to 32 bytes (8 DWORDS). This is required when time stamping
  // is activated above.
  // XXX I believe that by default the enhanced descriptor size is always
  // XXX used in the HAL Ethernet driver. This assumpton will need to be
  // XXX verified. Otherwise HAL_ETH_ConfigDMA() may need to be called. */
  // ETH_EnhancedDescriptorCmd(ENABLE);

  // The Time stamp counter starts operation as soon as it is initialized
  // with the value written in the Time stamp update register.
}

// Get the PTP time.
void ethptp_get_time(ptptime_t *timestamp)
{
  int32_t hi_reg;
  int32_t lo_reg;
  int32_t hi_reg_after;

  // The problem is we are reading two 32-bit registers that form
  // a 64-bit value, but it's possible the high 32-bits of the value
  // rolls over before we read the low 32-bits of the value.  To avoid
  // this situation we read the high 32-bits twice and determine which
  // high 32-bits the low 32-bit are associated with.
  __disable_irq();
  hi_reg = ETH_GetPTPRegister(ETH_PTPTSHR);
  lo_reg = ETH_GetPTPRegister(ETH_PTPTSLR);
  hi_reg_after = ETH_GetPTPRegister(ETH_PTPTSHR);
  __enable_irq();

  // Did a roll over occur while reading?
  if (hi_reg != hi_reg_after)
  {
    // We now know a roll over occurred. If the rollover occured before
    // the reading of the low 32-bits we move the substitute the second
    // 32-bit high value for the first 32-bit high value.
    if (lo_reg < (INT_MAX / 2)) hi_reg = hi_reg_after;
  }

  // Now convert the raw registers values into timestamp values.
  timestamp->tv_nsec = subsecond_to_nanosecond(lo_reg);
  timestamp->tv_sec = hi_reg;
}

// Set the PTP time.
void ethptp_set_time(ptptime_t *timestamp)
{
  uint32_t sign;
  uint32_t second_value;
  uint32_t nanosecond_value;
  uint32_t subsecond_value;

  // Determine sign and correct second and nanosecond values.
  if (timestamp->tv_sec < 0 || (timestamp->tv_sec == 0 && timestamp->tv_nsec < 0))
  {
    sign = ETH_PTP_NegativeTime;
    second_value = -timestamp->tv_sec;
    nanosecond_value = -timestamp->tv_nsec;
  }
  else
  {
    sign = ETH_PTP_PositiveTime;
    second_value = timestamp->tv_sec;
    nanosecond_value = timestamp->tv_nsec;
  }

  // Convert nanosecond to subseconds.
  subsecond_value = nanosecond_to_subsecond(nanosecond_value);

  // Write the offset (positive or negative) in the Time stamp update
  // high and low registers.
  ETH_SetPTPTimeStampUpdate(sign, second_value, subsecond_value);

  // Set Time stamp control register bit 2 (Time stamp init).
  ETH_InitializePTPTimeStamp();

  // The Time stamp counter starts operation as soon as it is initialized
  // with the value written in the Time stamp update register.
  while (ETH_GetPTPFlagStatus(ETH_PTP_FLAG_TSSTI) == SET);
}

// Adjust the PTP system clock rate by the specified value in parts-per-billion.
void ethptp_adj_freq(int32_t adj_ppb)
{
  // Adjust the fixed base frequency by parts-per-billion.
  // addend = base + ((base * adj_ppb) / 1000000000);
  uint32_t addend = ADJ_FREQ_BASE_ADDEND + (int32_t) ((((int64_t) ADJ_FREQ_BASE_ADDEND) * adj_ppb) / 1000000000);

  // Set the time stamp addend register with new rate value and set ETH_TPTSCR.
  ETH_SetPTPTimeStampAddend(addend);
  ETH_EnablePTPTimeStampAddend();
}

