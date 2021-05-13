#include "cmsis_os2.h"
#include "rtx_os.h"
#include "hal_system.h"
#include "syslog.h"
#include "i2c.h"

// I2C1 GPIO Configuration  
//
// PB8   ------> I2C1_SCL
// PB9   ------> I2C1_SDA 

// I2C handle.
static I2C_HandleTypeDef hi2c;

// Initialize the I2C peripheral.
void i2c_init(void)
{
  // GPIO peripheral clock enable.
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // I2C1 GPIO configuration.
  GPIO_InitTypeDef gpio_init;
  gpio_init.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  gpio_init.Mode = GPIO_MODE_AF_OD;
  gpio_init.Pull = GPIO_PULLUP;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &gpio_init);

  // I2C1 peripheral clock enable.
  __HAL_RCC_I2C1_CLK_ENABLE();

  // Configure the I2C port.
  hi2c.Instance = I2C1;
  hi2c.Init.Timing = 0x00303D5B;
  hi2c.Init.OwnAddress1 = 0;
  hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c.Init.OwnAddress2 = 0;
  hi2c.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c) != HAL_OK)
  {
    // Log the critical error.
    syslog_printf(SYSLOG_CRITICAL, "I2C: failed HAL I2C initialization");
  }

  // Configure analog filter.
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    // Log the critical error.
    syslog_printf(SYSLOG_CRITICAL, "I2C: failed HAL I2C analog filter configuration");
  }

  // Configure Digital filter.
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c, 0) != HAL_OK)
  {
    // Log the critical error.
    syslog_printf(SYSLOG_CRITICAL, "I2C: failed HAL I2C digital filter configuration");
  }
}

// Write data to the specified I2C device at the specified register address.
int i2c_register_write(uint8_t dev_address, uint8_t reg_address, uint8_t *buffer, uint16_t buflen)
{
  // We timeout after 5 milliseconds.
  HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c, dev_address, reg_address, I2C_MEMADD_SIZE_8BIT, buffer, buflen, 5);

  // Return 0 if OK, otherwise -1 to indicate an error.
  return status == HAL_OK ? 0 : -1;
}

// Read data from the specified I2C device at the specified register address.
int i2c_register_read(uint8_t dev_address, uint8_t reg_address, uint8_t *buffer, uint16_t buflen)
{
  // We timeout after 5 milliseconds.
  HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c, dev_address, reg_address, I2C_MEMADD_SIZE_8BIT, buffer, buflen, 5);

  // Return 0 if OK, otherwise -1 to indicate an error.
  return status == HAL_OK ? 0 : -1;
}
