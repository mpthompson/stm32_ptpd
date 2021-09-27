#ifndef __MEMORY_H__
#define __MEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif

// Base address of the STM32F42xxx flash sectors -- bank 1.
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) // Base @ of Sector 0, 16 Kbyte
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) // Base @ of Sector 1, 16 Kbyte
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) // Base @ of Sector 2, 16 Kbyte
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) // Base @ of Sector 3, 16 Kbyte
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) // Base @ of Sector 4, 64 Kbyte
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) // Base @ of Sector 5, 128 Kbyte
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) // Base @ of Sector 6, 128 Kbyte
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) // Base @ of Sector 7, 128 Kbyte
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) // Base @ of Sector 8, 128 Kbyte
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) // Base @ of Sector 9, 128 Kbyte
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) // Base @ of Sector 10, 128 Kbyte
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) // Base @ of Sector 11, 128 Kbyte

// Base address of the STM32F42xxx flash sectors -- bank 2.
#define ADDR_FLASH_SECTOR_12    ((uint32_t)0x08100000) // Base @ of Sector 12, 16 Kbyte
#define ADDR_FLASH_SECTOR_13    ((uint32_t)0x08104000) // Base @ of Sector 13, 16 Kbyte
#define ADDR_FLASH_SECTOR_14    ((uint32_t)0x08108000) // Base @ of Sector 14, 16 Kbyte
#define ADDR_FLASH_SECTOR_15    ((uint32_t)0x0810C000) // Base @ of Sector 15, 16 Kbyte
#define ADDR_FLASH_SECTOR_16    ((uint32_t)0x08110000) // Base @ of Sector 16, 64 Kbyte
#define ADDR_FLASH_SECTOR_17    ((uint32_t)0x08120000) // Base @ of Sector 17, 128 Kbyte
#define ADDR_FLASH_SECTOR_18    ((uint32_t)0x08140000) // Base @ of Sector 18, 128 Kbyte
#define ADDR_FLASH_SECTOR_19    ((uint32_t)0x08160000) // Base @ of Sector 19, 128 Kbyte
#define ADDR_FLASH_SECTOR_20    ((uint32_t)0x08180000) // Base @ of Sector 20, 128 Kbyte
#define ADDR_FLASH_SECTOR_21    ((uint32_t)0x081A0000) // Base @ of Sector 21, 128 Kbyte
#define ADDR_FLASH_SECTOR_22    ((uint32_t)0x081C0000) // Base @ of Sector 22, 128 Kbyte
#define ADDR_FLASH_SECTOR_23    ((uint32_t)0x081E0000) // Base @ of Sector 23, 128 Kbyte

// Size of the STM32F42xxx flash sectors -- bank 1.
#define SIZE_FLASH_SECTOR_0     ((uint32_t)0x00004000) // 16 Kbyte
#define SIZE_FLASH_SECTOR_1     ((uint32_t)0x00004000) // 16 Kbyte
#define SIZE_FLASH_SECTOR_2     ((uint32_t)0x00004000) // 16 Kbyte
#define SIZE_FLASH_SECTOR_3     ((uint32_t)0x00004000) // 16 Kbyte
#define SIZE_FLASH_SECTOR_4     ((uint32_t)0x00010000) // 64 Kbyte
#define SIZE_FLASH_SECTOR_5     ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_6     ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_7     ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_8     ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_9     ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_10    ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_11    ((uint32_t)0x00020000) // 128 Kbyte

// Size of the STM32F42xxx flash sectors -- bank 2.
#define SIZE_FLASH_SECTOR_12    ((uint32_t)0x00004000) // 16 Kbyte
#define SIZE_FLASH_SECTOR_13    ((uint32_t)0x00004000) // 16 Kbyte
#define SIZE_FLASH_SECTOR_14    ((uint32_t)0x00004000) // 16 Kbyte
#define SIZE_FLASH_SECTOR_15    ((uint32_t)0x00004000) // 16 Kbyte
#define SIZE_FLASH_SECTOR_16    ((uint32_t)0x00010000) // 64 Kbyte
#define SIZE_FLASH_SECTOR_17    ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_18    ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_19    ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_20    ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_21    ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_22    ((uint32_t)0x00020000) // 128 Kbyte
#define SIZE_FLASH_SECTOR_23    ((uint32_t)0x00020000) // 128 Kbyte

#define TYPE_OFFSET                       (0x200)
#define VER_MAJOR_OFFSET                  (0x201)
#define VER_MINOR_OFFSET                  (0x202)
#define INFO_STRING_OFFSET                (0x210)

// User bootloader executes in sectors 0 thru 4.
#define BOOT_FLASH_START_SECTOR           0
#define BOOT_FLASH_END_SECTOR             4

// Define the address from where bootloader application will be loaded.
#define BOOT_FLASH_START_ADDRESS          ADDR_FLASH_SECTOR_0
#define BOOT_FLASH_END_ADDRESS            ((uint32_t)0x0801FFFF)

#define BOOT_FLASH_TYPE_ADDRESS           (BOOT_FLASH_START_ADDRESS + TYPE_OFFSET)
#define BOOT_FLASH_VER_MAJOR_ADDRESS      (BOOT_FLASH_START_ADDRESS + VER_MAJOR_OFFSET)
#define BOOT_FLASH_VER_MINOR_ADDRESS      (BOOT_FLASH_START_ADDRESS + VER_MINOR_OFFSET)
#define BOOT_FLASH_INFO_STRING_ADDRESS    (BOOT_FLASH_START_ADDRESS + INFO_STRING_OFFSET)

// User application executes in sectors 5 thru 23.
#define USER_FLASH_START_SECTOR           5
#define USER_FLASH_END_SECTOR             23

// Define the address from where user application will be loaded.
#define USER_FLASH_START_ADDRESS          ADDR_FLASH_SECTOR_5
#define USER_FLASH_END_ADDRESS            ((uint32_t)0x081EFFFF)

#define USER_FLASH_APPLICATON             USER_FLASH_START_ADDRESS

#define USER_FLASH_TYPE_ADDRESS           (USER_FLASH_START_ADDRESS + TYPE_OFFSET)
#define USER_FLASH_VER_MAJOR_ADDRESS      (USER_FLASH_START_ADDRESS + VER_MAJOR_OFFSET)
#define USER_FLASH_VER_MINOR_ADDRESS      (USER_FLASH_START_ADDRESS + VER_MINOR_OFFSET)
#define USER_FLASH_INFO_STRING_ADDRESS    (USER_FLASH_START_ADDRESS + INFO_STRING_OFFSET)

// Define the user application size.
#define USER_FLASH_SIZE                   (USER_FLASH_END_ADDRESS - USER_FLASH_START_ADDRESS + 1)

#ifdef __cplusplus
}
#endif

#endif /* __MEMORY_H__ */
