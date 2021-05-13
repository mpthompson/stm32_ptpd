#ifndef __WEBCLIENT_H__
#define __WEBCLIENT_H__

#include <stdint.h>
#include "lwip/ip4_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  WEBCLIENT_STS_OK = 0,
  WEBCLIENT_STS_TIMEOUT,
  WEBCLIENT_STS_NOTFOUND,
  WEBCLIENT_STS_ERROR
};

typedef void (*webclient_func_t)(uint32_t status, const uint8_t *buffer, uint32_t buflen);

void webclient_init(void);
int webclient_get(ip4_addr_t addr, uint16_t port, const char *path, webclient_func_t func);

#ifdef __cplusplus
}
#endif

#endif /* __WEBCLIENT_H__ */
