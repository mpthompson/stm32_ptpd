#ifndef __MEMORY_H__
#define __MEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif

// Base address of single bank flash sectors.
#define ADDR_FLASH_SB_SECTOR_0   ((uint32_t)0x08000000) /* Base address of Sector 0, 32 Kbytes */
#define ADDR_FLASH_SB_SECTOR_1   ((uint32_t)0x08008000) /* Base address of Sector 1, 32 Kbytes */
#define ADDR_FLASH_SB_SECTOR_2   ((uint32_t)0x08010000) /* Base address of Sector 2, 32 Kbytes */
#define ADDR_FLASH_SB_SECTOR_3   ((uint32_t)0x08018000) /* Base address of Sector 3, 32 Kbytes */
#define ADDR_FLASH_SB_SECTOR_4   ((uint32_t)0x08020000) /* Base address of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SB_SECTOR_5   ((uint32_t)0x08040000) /* Base address of Sector 5, 256 Kbytes */
#define ADDR_FLASH_SB_SECTOR_6   ((uint32_t)0x08080000) /* Base address of Sector 6, 256 Kbytes */
#define ADDR_FLASH_SB_SECTOR_7   ((uint32_t)0x080C0000) /* Base address of Sector 7, 256 Kbytes */
#define ADDR_FLASH_SB_SECTOR_8   ((uint32_t)0x08100000) /* Base address of Sector 8, 256 Kbytes */
#define ADDR_FLASH_SB_SECTOR_9   ((uint32_t)0x08140000) /* Base address of Sector 9, 256 Kbytes */
#define ADDR_FLASH_SB_SECTOR_10  ((uint32_t)0x08180000) /* Base address of Sector 10, 256 Kbytes */
#define ADDR_FLASH_SB_SECTOR_11  ((uint32_t)0x081C0000) /* Base address of Sector 11, 256 Kbytes */

// Base address of dual bank flash sectors.
#define ADDR_FLASH_DB_SECTOR_0   ((uint32_t)0x08000000) /* Base address of Sector 0, 16 Kbytes */
#define ADDR_FLASH_DB_SECTOR_1   ((uint32_t)0x08004000) /* Base address of Sector 1, 16 Kbytes */
#define ADDR_FLASH_DB_SECTOR_2   ((uint32_t)0x08008000) /* Base address of Sector 2, 16 Kbytes */
#define ADDR_FLASH_DB_SECTOR_3   ((uint32_t)0x0800C000) /* Base address of Sector 3, 16 Kbytes */
#define ADDR_FLASH_DB_SECTOR_4   ((uint32_t)0x08010000) /* Base address of Sector 4, 64 Kbytes */
#define ADDR_FLASH_DB_SECTOR_5   ((uint32_t)0x08020000) /* Base address of Sector 5, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_6   ((uint32_t)0x08040000) /* Base address of Sector 6, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_7   ((uint32_t)0x08060000) /* Base address of Sector 7, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_8   ((uint32_t)0x08080000) /* Base address of Sector 8, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_9   ((uint32_t)0x080A0000) /* Base address of Sector 9, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_10  ((uint32_t)0x080C0000) /* Base address of Sector 10, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_11  ((uint32_t)0x080E0000) /* Base address of Sector 11, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_12  ((uint32_t)0x08100000) /* Base address of Sector 12, 16 Kbytes */
#define ADDR_FLASH_DB_SECTOR_13  ((uint32_t)0x08104000) /* Base address of Sector 13, 16 Kbytes */
#define ADDR_FLASH_DB_SECTOR_14  ((uint32_t)0x08108000) /* Base address of Sector 14, 16 Kbytes */
#define ADDR_FLASH_DB_SECTOR_15  ((uint32_t)0x0810C000) /* Base address of Sector 15, 16 Kbytes */
#define ADDR_FLASH_DB_SECTOR_16  ((uint32_t)0x08110000) /* Base address of Sector 16, 64 Kbytes */
#define ADDR_FLASH_DB_SECTOR_17  ((uint32_t)0x08120000) /* Base address of Sector 17, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_18  ((uint32_t)0x08140000) /* Base address of Sector 18, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_19  ((uint32_t)0x08160000) /* Base address of Sector 19, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_20  ((uint32_t)0x08180000) /* Base address of Sector 20, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_21  ((uint32_t)0x081A0000) /* Base address of Sector 21, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_22  ((uint32_t)0x081C0000) /* Base address of Sector 22, 128 Kbytes */
#define ADDR_FLASH_DB_SECTOR_23  ((uint32_t)0x081E0000) /* Base address of Sector 23, 128 Kbytes */

// User bootloader in dual bank mode executes in sectors 0 thru 3.
#define BOOT_FLASH_SB_START_SECTOR        0
#define BOOT_FLASH_SB_END_SECTOR          3

// User bootloader in dual bank mode executes in sectors 0 thru 4.
#define BOOT_FLASH_DB_START_SECTOR        0
#define BOOT_FLASH_DB_END_SECTOR          4

// Define the address from where bootloader application will be loaded.
#define BOOT_FLASH_START_ADDRESS          ((uint32_t)0x08000000)
#define BOOT_FLASH_END_ADDRESS            ((uint32_t)0x0801FFFF)

// User application in dual bank mode executes in sectors 4 thru 11.
#define USER_FLASH_SB_START_SECTOR        4
#define USER_FLASH_SB_END_SECTOR          11

// User application in dual bank mode executes in sectors 5 thru 23.
#define USER_FLASH_DB_START_SECTOR        5
#define USER_FLASH_DB_END_SECTOR          23

// Define the address from where user application will be loaded.
#define USER_FLASH_START_ADDRESS          ((uint32_t)0x08020000)
#define USER_FLASH_END_ADDRESS            ((uint32_t)0x081EFFFF)

// Define the user application flash address and size.
#define USER_FLASH_APPLICATON             USER_FLASH_START_ADDRESS
#define USER_FLASH_SIZE                   (USER_FLASH_END_ADDRESS - USER_FLASH_START_ADDRESS + 1)

#ifdef __cplusplus
}
#endif

#endif /* __MEMORY_H__ */
