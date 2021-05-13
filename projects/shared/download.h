#ifndef __DOWNLOAD_H__
#define __DOWNLOAD_H__

#include <stdint.h>
#include "lwip/ip4_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

void download_init(void);

// System configurable functions to be used for download configuration.
// Implemented as weak functions.
ip4_addr_t download_config_address(void);
uint16_t download_config_port(void);
const char *download_config_firmware_name(void);

#ifdef __cplusplus
}
#endif

#endif /* __DOWNLOAD_H__ */
