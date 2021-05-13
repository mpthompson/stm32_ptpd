#ifndef _CKSUM_H
#define _CKSUM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void cksum_init(void);
uint32_t cksum_get_crc(void);
uint32_t cksum_get_length(void);

#ifdef __cplusplus
}
#endif

#endif /* _CKSUM_H */
