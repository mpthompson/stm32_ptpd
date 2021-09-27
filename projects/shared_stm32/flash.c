#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "cmsis_os2.h"
#include "hal_system.h"
#include "crc.h"
#include "syslog.h"
#include "memory.h"
#include "network.h"
#include "console.h"
#include "flash.h"

static const uint32_t flash_sector_addr[FLASH_SECTOR_TOTAL] =
{
  ADDR_FLASH_SECTOR_0,
  ADDR_FLASH_SECTOR_1,
  ADDR_FLASH_SECTOR_2,
  ADDR_FLASH_SECTOR_3,
  ADDR_FLASH_SECTOR_4,
  ADDR_FLASH_SECTOR_5,
  ADDR_FLASH_SECTOR_6,
  ADDR_FLASH_SECTOR_7,
  ADDR_FLASH_SECTOR_8,
  ADDR_FLASH_SECTOR_9,
  ADDR_FLASH_SECTOR_10,
  ADDR_FLASH_SECTOR_11,
  ADDR_FLASH_SECTOR_12,
  ADDR_FLASH_SECTOR_13,
  ADDR_FLASH_SECTOR_14,
  ADDR_FLASH_SECTOR_15,
  ADDR_FLASH_SECTOR_16,
  ADDR_FLASH_SECTOR_17,
  ADDR_FLASH_SECTOR_18,
  ADDR_FLASH_SECTOR_19,
  ADDR_FLASH_SECTOR_20,
  ADDR_FLASH_SECTOR_21,
  ADDR_FLASH_SECTOR_22,
  ADDR_FLASH_SECTOR_23
};

static const uint32_t flash_sector_size[FLASH_SECTOR_TOTAL] =
{
  SIZE_FLASH_SECTOR_0,
  SIZE_FLASH_SECTOR_1,
  SIZE_FLASH_SECTOR_2,
  SIZE_FLASH_SECTOR_3,
  SIZE_FLASH_SECTOR_4,
  SIZE_FLASH_SECTOR_5,
  SIZE_FLASH_SECTOR_6,
  SIZE_FLASH_SECTOR_7,
  SIZE_FLASH_SECTOR_8,
  SIZE_FLASH_SECTOR_9,
  SIZE_FLASH_SECTOR_10,
  SIZE_FLASH_SECTOR_11,
  SIZE_FLASH_SECTOR_12,
  SIZE_FLASH_SECTOR_13,
  SIZE_FLASH_SECTOR_14,
  SIZE_FLASH_SECTOR_15,
  SIZE_FLASH_SECTOR_16,
  SIZE_FLASH_SECTOR_17,
  SIZE_FLASH_SECTOR_18,
  SIZE_FLASH_SECTOR_19,
  SIZE_FLASH_SECTOR_20,
  SIZE_FLASH_SECTOR_21,
  SIZE_FLASH_SECTOR_22,
  SIZE_FLASH_SECTOR_23
};

// Application firmware flash writing state variables.
static uint32_t flash_app_addr;
static uint32_t flash_app_data;
static uint32_t flash_app_count;
static bool flash_app_error;

// Returns true if the indicated area of flash is erased.
static bool flash_sector_erased(uint32_t sector)
{
  uint8_t *p = (uint8_t *) flash_sector_addr[sector];
  uint32_t size = flash_sector_size[sector];
  
  // Check across the flash sector that all bytes are 0xFF.
  while (size--)
  {
    // Return false if the byte in flash is not erased.
    if (*(p++) != 0xFF) return false;
  }

  // Yes, the sector is erased.
  return true;
}

// Erase the indicated flash sector.
static int flash_erase_sector(uint32_t sector)
{
  HAL_StatusTypeDef status;
  uint32_t sector_error;

  // Sanity check the sector.
  if (sector > FLASH_SECTOR_23) return 0;
  
  // Don't bother if the sector is already erased.  This helps
  // save on wear and tear on the flash memory.
  if (flash_sector_erased(sector)) return 0;

  // Unlock the flash memory for writing.
  HAL_FLASH_Unlock();

  // Erase the specified flash Sector.
  FLASH_EraseInitTypeDef erase_init;
  memset(&erase_init, 0, sizeof(erase_init));
  erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_init.Banks = 0;
  erase_init.Sector = sector;
  erase_init.NbSectors = 1;
  erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  status = HAL_FLASHEx_Erase(&erase_init, &sector_error);

  // Lock the flash memory from writing.
  HAL_FLASH_Lock();

  return status == HAL_OK ? 0 : -1;
}

// Initialize the flash driver.
void flash_init(void)
{
  // Do nothing for now.
}

// Get the CRC value application firmware across the indicated number of bytes.
uint32_t flash_app_crc(uint32_t length)
{
  uint32_t crc_sum = 0;

  // Sanity check the length to not be greater than the total size
  // of application flash memory.
  if (length > USER_FLASH_SIZE) length = USER_FLASH_SIZE;

  // Determine the CRC across the application flash memory.
  crc_sum = crc32_process(crc_sum, (uint8_t *) USER_FLASH_START_ADDRESS, length);

  // Extend with the length of the flash memory.
  crc_sum = crc32_extend(crc_sum, length);

  return crc_sum;
}

// Erase the flash sectors associated with application firmware.
int flash_app_erase(void)
{
  uint32_t sector;

  // Loop over each of the application flash sectors.
  for (sector = USER_FLASH_START_SECTOR; sector <= USER_FLASH_END_SECTOR; ++sector)
  {
    // Erase the flash sector.
    if (flash_erase_sector(sector) != 0)
    {
      // Log the error.
      syslog_printf(SYSLOG_CRITICAL, "FLASH: Error erasing application sector %u at address 0x%08x",
                    sector, flash_sector_addr[sector]);

      // Return an error.
      return -1;
    }
  }

  return 0;
}

// Start the writing of application firmware to flash.
int flash_app_prog_start(uint32_t length)
{
  // Sanity check the length to not be greater than the total size
  // of application flash memory.
  if (length > USER_FLASH_SIZE)
  {
    // Remember that we saw an error.
    flash_app_error = true;

    // Return an error.
    return -1;
  }

  // Initialize the flash app write information.
  flash_app_addr = USER_FLASH_START_ADDRESS;
  flash_app_data = 0;
  flash_app_count = 0;
  flash_app_error = false;

  // Unlock the flash memory for writing.
  HAL_FLASH_Unlock();

  // We succeeded.
  return 0;
}

// Write a block of application firmware to flash.
int flash_app_prog_write(const uint8_t *buffer, uint32_t buflen)
{
  HAL_StatusTypeDef status;

  // Sanity check that we haven't any errors.
  if (flash_app_error) return -1;

  // Loop over each byte in the buffer.
  while (buflen)
  {
    // Update the data word.
    ((uint8_t *) &flash_app_data)[flash_app_count] = *buffer;

    // Increment the count.
    flash_app_count += 1;

    // Do we have data to write?
    if (flash_app_count == 4)
    {
      // Write word at the update address to the user address.
      status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_app_addr, flash_app_data);

      // Check for failure to program the word?
      if (status != HAL_OK)
      {
        // Remember that we saw an error.
        flash_app_error = true;

        // Log the critical error.
        syslog_printf(SYSLOG_CRITICAL, "FLASH: Error programming flash word at address 0x%08x.\n", flash_app_addr);

        // Return an error.
        return -1;
      }

      // Reset the app data and count.
      flash_app_data = 0;
      flash_app_count = 0;

      // Increment the user address.
      flash_app_addr += 4;
    }

    // Increment the buffer and decrement the length.
    buffer += 1;
    buflen -= 1;
  }

  // We succeeded.
  return 0;
}

// End the writing of application firmware to flash.
int flash_app_prog_end(void)
{
  HAL_StatusTypeDef status;

  // Is there any remaining data to write?
  if (!flash_app_error && (flash_app_count > 0))
  {
    // Fill in the rest of the word to write.
    while (flash_app_count < 4)
    {
      // Update the data word.
      ((uint8_t *) &flash_app_data)[flash_app_count] = 0xff;
 
      // Increment the count.
      flash_app_count += 1;
    }

    // Write word at the update address to the user address.
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_app_addr, flash_app_data);

    // Check for failure to program the word?
    if (status != HAL_OK)
    {
      // Remember that we saw an error.
      flash_app_error = true;

      // Log the critical error.
      syslog_printf(SYSLOG_CRITICAL, "FLASH: Error programming flash word at address 0x%08x.\n", flash_app_addr);
    }

    // Reset the app data and count.
    flash_app_data = 0;
    flash_app_count = 0;

    // Increment the user address.
    flash_app_addr += 4;
  }

  // Lock the flash memory from writing.
  HAL_FLASH_Lock();

  // We succeeded.
  return 0;
}

// Run the application firmware.
int flash_app_run(void)
{
  // IMPORTANT: Be careful of using any local variables in this function because
  // of the stack munging in the code below.  You have been warned.
  uint32_t firmware_jump_addr;

  // Get the application firmware jump address.
  firmware_jump_addr = *(__IO uint32_t *) (USER_FLASH_APPLICATON + 4);

  // Check Vector Table: Test if user code is programmed starting from address "USER_FLASH_APPLICATON" 
  if ((firmware_jump_addr >= USER_FLASH_START_ADDRESS) && (firmware_jump_addr <= USER_FLASH_END_ADDRESS))
  {
    // Sleep for 200 milliseconds.
    sys_msleep(200);

    // Suspend the OS.
    osKernelSuspend();

    // Stop network activity.
    network_deinit();

    // Deinitialzie the console UART.
    console_deinit();

    // Suspend the HAL system tick.
    HAL_SuspendTick();

    // Set the application vector table address.
    SCB->VTOR = (uint32_t) USER_FLASH_APPLICATON;

    // Initialize user application's stack pointer. Be very careful here
    // because we loose access to local variables when modifying stacks.
    __set_MSP(*(__IO uint32_t*) USER_FLASH_APPLICATON);
    __set_PSP(*(__IO uint32_t*) USER_FLASH_APPLICATON);

    // Set Privileged Thread mode & MSP.
    __set_CONTROL(0x00U);
    __DSB();
    __ISB();

    // Jump to the application.  I know, this is probably the most hideious
    // piece of C code you'll ever lay your eyes on.  It is implemented this
    // way to jump to the code pointed to by the reset vector of the
    // application firmware without the need for intermediate variables.
    // Use of variables after modification of the stack pointers would be
    // dangerous and this hideous code accomplishes our goal.
    ((void (*)(void)) (*(uint32_t *) (USER_FLASH_APPLICATON + 4)))();
  }

  // If we get here it must be an error.
  return -1;
}
