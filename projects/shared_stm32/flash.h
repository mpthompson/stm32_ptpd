#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void flash_init(void);
uint32_t flash_app_crc(uint32_t length);
int flash_app_erase(void);
int flash_app_prog_start(uint32_t length);
int flash_app_prog_write(const uint8_t *buffer, uint32_t buflen);
int flash_app_prog_end(void);
int flash_app_run(void);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_H__ */
