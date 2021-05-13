#ifndef __CRC_H__
#define __CRC_H__

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc16_process(uint16_t crc, const uint8_t *buffer, size_t buflen);
uint32_t crc32_process(uint32_t crc, const uint8_t *buffer, size_t buflen);
uint32_t crc32_extend(uint32_t crc, uint32_t filesize);

#ifdef __cplusplus
}
#endif

#endif /* __CRC_H__ */
