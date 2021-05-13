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

// Application firmware flash writing state variables.
static bool flash_sb_mode = false;
static uint32_t flash_app_addr;
static uint32_t flash_app_data;
static uint32_t flash_app_count;
static bool flash_app_error;

// Get the sector of a given address.
static uint32_t flash_sector_address(uint32_t address)
{
  uint32_t sector = 0;

  if (!flash_sb_mode)
  {
    if ((address < ADDR_FLASH_SB_SECTOR_1) && (address >= ADDR_FLASH_SB_SECTOR_0))
    {
      sector = FLASH_SECTOR_0;
    }
    else if ((address < ADDR_FLASH_SB_SECTOR_2) && (address >= ADDR_FLASH_SB_SECTOR_1))
    {
      sector = FLASH_SECTOR_1;
    }
    else if ((address < ADDR_FLASH_SB_SECTOR_3) && (address >= ADDR_FLASH_SB_SECTOR_2))
    {
      sector = FLASH_SECTOR_2;
    }
    else if ((address < ADDR_FLASH_SB_SECTOR_4) && (address >= ADDR_FLASH_SB_SECTOR_3))
    {
      sector = FLASH_SECTOR_3;
    }
    else if ((address < ADDR_FLASH_SB_SECTOR_5) && (address >= ADDR_FLASH_SB_SECTOR_4))
    {
      sector = FLASH_SECTOR_4;
    }
    else if ((address < ADDR_FLASH_SB_SECTOR_6) && (address >= ADDR_FLASH_SB_SECTOR_5))
    {
      sector = FLASH_SECTOR_5;
    }
    else if ((address < ADDR_FLASH_SB_SECTOR_7) && (address >= ADDR_FLASH_SB_SECTOR_6))
    {
      sector = FLASH_SECTOR_6;
    }
    else if ((address < ADDR_FLASH_SB_SECTOR_8) && (address >= ADDR_FLASH_SB_SECTOR_7))
    {
      sector = FLASH_SECTOR_7;
    }
    else if ((address < ADDR_FLASH_SB_SECTOR_9) && (address >= ADDR_FLASH_SB_SECTOR_8))
    {
      sector = FLASH_SECTOR_8;
    }
    else if ((address < ADDR_FLASH_SB_SECTOR_10) && (address >= ADDR_FLASH_SB_SECTOR_9))
    {
      sector = FLASH_SECTOR_9;
    }
    else if ((address < ADDR_FLASH_SB_SECTOR_11) && (address >= ADDR_FLASH_SB_SECTOR_10))
    {
      sector = FLASH_SECTOR_10;
    }
    else // (address < FLASH_END_ADDR) && (address >= ADDR_FLASH_SB_SECTOR_11)
    {
      sector = FLASH_SECTOR_11;
    }
  }
  else
  {
    if ((address < ADDR_FLASH_DB_SECTOR_1) && (address >= ADDR_FLASH_DB_SECTOR_0))
    {
      sector = FLASH_SECTOR_0;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_2) && (address >= ADDR_FLASH_DB_SECTOR_1))
    {
      sector = FLASH_SECTOR_1;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_3) && (address >= ADDR_FLASH_DB_SECTOR_2))
    {
      sector = FLASH_SECTOR_2;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_4) && (address >= ADDR_FLASH_DB_SECTOR_3))
    {
      sector = FLASH_SECTOR_3;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_5) && (address >= ADDR_FLASH_DB_SECTOR_4))
    {
      sector = FLASH_SECTOR_4;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_6) && (address >= ADDR_FLASH_DB_SECTOR_5))
    {
      sector = FLASH_SECTOR_5;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_7) && (address >= ADDR_FLASH_DB_SECTOR_6))
    {
      sector = FLASH_SECTOR_6;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_8) && (address >= ADDR_FLASH_DB_SECTOR_7))
    {
      sector = FLASH_SECTOR_7;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_9) && (address >= ADDR_FLASH_DB_SECTOR_8))
    {
      sector = FLASH_SECTOR_8;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_10) && (address >= ADDR_FLASH_DB_SECTOR_9))
    {
      sector = FLASH_SECTOR_9;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_11) && (address >= ADDR_FLASH_DB_SECTOR_10))
    {
      sector = FLASH_SECTOR_10;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_12) && (address >= ADDR_FLASH_DB_SECTOR_11))
    {
      sector = FLASH_SECTOR_11;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_13) && (address >= ADDR_FLASH_DB_SECTOR_12))
    {
      sector = FLASH_SECTOR_12;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_14) && (address >= ADDR_FLASH_DB_SECTOR_13))
    {
      sector = FLASH_SECTOR_13;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_15) && (address >= ADDR_FLASH_DB_SECTOR_14))
    {
      sector = FLASH_SECTOR_14;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_16) && (address >= ADDR_FLASH_DB_SECTOR_15))
    {
      sector = FLASH_SECTOR_15;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_17) && (address >= ADDR_FLASH_DB_SECTOR_16))
    {
      sector = FLASH_SECTOR_16;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_18) && (address >= ADDR_FLASH_DB_SECTOR_17))
    {
      sector = FLASH_SECTOR_17;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_19) && (address >= ADDR_FLASH_DB_SECTOR_18))
    {
      sector = FLASH_SECTOR_18;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_20) && (address >= ADDR_FLASH_DB_SECTOR_19))
    {
      sector = FLASH_SECTOR_19;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_21) && (address >= ADDR_FLASH_DB_SECTOR_20))
    {
      sector = FLASH_SECTOR_20;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_22) && (address >= ADDR_FLASH_DB_SECTOR_21))
    {
      sector = FLASH_SECTOR_21;
    }
    else if ((address < ADDR_FLASH_DB_SECTOR_23) && (address >= ADDR_FLASH_DB_SECTOR_22))
    {
      sector = FLASH_SECTOR_22;
    }
    else // (address < FLASH_END_ADDR) && (address >= ADDR_FLASH_DB_SECTOR_23)
    {
      sector = FLASH_SECTOR_23;
    }
  }

  return sector;
}

// Initialize the flash driver.
void flash_init(void)
{
  FLASH_OBProgramInitTypeDef ob_init; 

  // Unlock to enable the flash control register access.
  HAL_FLASH_Unlock();

  // Allow access to option bytes sector.
  HAL_FLASH_OB_Unlock();

  // Get the Dual bank configuration status.
  HAL_FLASHEx_OBGetConfig(&ob_init);

  // Are we in single bank or dual bank mode?
  flash_sb_mode = (ob_init.USERConfig & OB_NDBANK_SINGLE_BANK) == OB_NDBANK_SINGLE_BANK ? true : false;

  // Lack to disable access to option bytes sector.
  HAL_FLASH_OB_Lock();

  // Lock to disable the flash control register access.
  HAL_FLASH_Lock();
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
// Returns 0 for success, -1 for error.
int flash_app_erase(void)
{
  uint32_t sector_error = 0;
  FLASH_EraseInitTypeDef erase_init;

  // Get the start and end sectors to be erased.
  uint32_t start_sector = flash_sb_mode ? USER_FLASH_SB_START_SECTOR : USER_FLASH_DB_START_SECTOR;
  uint32_t end_sector = flash_sb_mode ? USER_FLASH_SB_END_SECTOR : USER_FLASH_DB_END_SECTOR;

  // Unlock to enable the flash control register access.
  HAL_FLASH_Unlock();

  // Fill in the erase init structure.
  erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  erase_init.Sector  = start_sector;
  erase_init.NbSectors = end_sector - start_sector + 1;

  // Note: If an erase operation in flash memory also concerns data in the data or instruction cache
  // we must make sure that these data are rewritten before they are accessed during code
  // execution. If this cannot be done safely, it is recommended to flush the caches by setting the
  // DCRST and ICRST bits in the FLASH_CR register.
  int status = HAL_FLASHEx_Erase(&erase_init, &sector_error) == HAL_OK ? 0 : -1;

  // Lock to disable the flash control register access.
  HAL_FLASH_Lock();

  // Log an error if we did not succeed.
  if (status != 0)
  {
    // Log the error.
    syslog_printf(SYSLOG_CRITICAL, "FLASH: Error erasing application sectors in %s-bank mode",
                  flash_sb_mode ? "single" : "dual");
  }

  return status;
}

// Start the writing of application firmware to flash.
// Returns 0 for success, -1 for error.
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
        syslog_printf(SYSLOG_CRITICAL, "FLASH: Error programming flash word at address 0x%08x.", flash_app_addr);

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
      syslog_printf(SYSLOG_CRITICAL, "FLASH: Error programming flash word at address 0x%08x.", flash_app_addr);
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
    __set_PSP(0);

    // Set Privileged Thread mode & MSP.
    __set_CONTROL(0x00U);
    __DSB();
    __ISB();

    // Clear out registers to mimic coming from reset.
    // __ASM volatile ("MOV R0, #0");
    // __ASM volatile ("MOV R1, #0");
    __ASM volatile ("MOV R2, #0");
    __ASM volatile ("MOV R3, #0");
    __ASM volatile ("MOV R4, #0");
    __ASM volatile ("MOV R5, #0");
    __ASM volatile ("MOV R6, #0");
    __ASM volatile ("MOV R7, #0");
    __ASM volatile ("MOV R8, #0");
    __ASM volatile ("MOV R9, #0");
    __ASM volatile ("MOV R10, #0");
    __ASM volatile ("MOV R11, #0");
    __ASM volatile ("MOV R12, #0");
    __ASM volatile ("MOV R14, #0xFFFF");
    __ASM volatile ("MOVT R14, #:lower16:0xFFFF");

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
