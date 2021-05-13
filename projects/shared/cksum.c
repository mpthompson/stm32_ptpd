#include <string.h>
#include "cmsis_os2.h"
#include "crc.h"
#include "shell.h"
#include "cksum.h"

// The intention here is that the CRC value from the cksum command should
// match the CRC of the .bin file programmed into flash memory as determined
// by the Linux cksum shell command.

static uint32_t cksum_crc = 0;
static uint32_t cksum_len = 0;

// The GNUCC and ARMCC differ in how they define the flash load region.
#if !defined(__ARMCC_VERSION) && defined (__GNUC__)
// These symbols are defined in the GNU linker script stm32_flash.ld.
extern uint32_t _sflash;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _sbss;
#else
// ARMCC Linker-defined symbols for the LR_IROM1 load region. This is the
// region of flash memory that defines this application. The assumption
// here is that LR_IROM1 is the correct load region to be used.
extern uint32_t Load$$LR$$LR_IROM1$$Base;
extern uint32_t Load$$LR$$LR_IROM1$$Length;
extern uint32_t Load$$LR$$LR_IROM1$$Limit;
#endif

// Address of the load region.
static uint32_t load_region_base(void)
{
#if !defined(__ARMCC_VERSION) && defined (__GNUC__)
  // Return the address of the base of flash memory from linker script symbols.
  uint32_t base = (uint32_t) (uintptr_t) &_sflash;
#else
  // The ARMCC linker generates the Load$$LR$$LR_IROM1$$Base symbol.
  uint32_t base = (uint32_t) (uintptr_t) &Load$$LR$$LR_IROM1$$Base;
#endif

  // Sanity check the base.
  if (base < 0x8000000) base = 0x8000000;
  if (base > 0x8200000) base = 0x8200000;

  return base;
}

// Length of the load region.
static uint32_t load_region_length(void)
{
#if !defined(__ARMCC_VERSION) && defined (__GNUC__)
  // Return the length of flash program memory from linker script symbols.
  uint32_t code_length = ((uint32_t) &_sidata) - ((uint32_t) &_sflash);
  uint32_t data_length = ((uint32_t) &_sbss) - ((uint32_t) &_sdata);
  uint32_t length = code_length + data_length;
#else
  // The ARMCC linker generates the Load$$LR$$LR_IROM1$$Length symbol.
  uint32_t length = (uint32_t) (uintptr_t) &Load$$LR$$LR_IROM1$$Length;
#endif

  // Sanity check the length.
  if (length > 0x1E0000) length = 0x1E0000;

  return length;
}

#if 0
// Address of the byte beyond the end of the load region.
static uint32_t load_region_limit(void)
{
#if !defined(__ARMCC_VERSION) && defined (__GNUC__)
  // Return the address of the byte of the flash program memory from linker script symbols.
  uint32_t limit = load_region_base() + load_region_length();
#else
  // The ARMCC linker generates the Load$$LR$$LR_IROM1$$Limit symbol.
  uint32_t limit = (uint32_t) (uintptr_t) &Load$$LR$$LR_IROM1$$Limit;
#endif
  // Sanity check the base.
  if (limit < 0x8000000) limit = 0x8000000;
  if (limit > 0x8200000) limit = 0x8200000;

  return limit;
}
#endif

// Checksum shell function.
static bool cksum_shell_cksum(int argc, char **argv)
{
#if 0
#if !defined(__ARMCC_VERSION) && defined (__GNUC__)
  // Display the checksum CRC and length.
  shell_printf("&_sflash=0x%08x &_sidata=0x%08x length=%u\n",
               (uint32_t) &_sflash, (uint32_t) &_sidata,
               ((uint32_t) &_sidata) - ((uint32_t) &_sflash));
  // Display the checksum CRC and length.
  shell_printf("&_sdata=0x%08x &_sbss=0x%08x length=%u\n",
               (uint32_t) &_sdata, (uint32_t) &_sbss,
               ((uint32_t) &_sbss) - ((uint32_t) &_sdata));
#endif
#endif

  // Display the checksum CRC and length.
  shell_printf("%u %u\n", cksum_get_crc(), cksum_get_length());

  return true;
}

// Initialize the checksum functions.
void cksum_init(void)
{
  cksum_crc = 0;
  cksum_len = 0;

  // Determine the CRC across the load region.
  cksum_crc = crc32_process(cksum_crc, (uint8_t *) load_region_base(), load_region_length());

  // Extend the CRC with the length of the load region.
  cksum_crc = crc32_extend(cksum_crc, load_region_length());

  // Determine the length of the load region.
  cksum_len = load_region_length();

  // Add the chsum command.
  shell_add_command("cksum", cksum_shell_cksum);
}

// Get the load region CRC value.
uint32_t cksum_get_crc(void)
{
  return cksum_crc;
}

// Get the load region length value.
uint32_t cksum_get_length(void)
{
  return cksum_len;
}



