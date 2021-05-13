#ifndef __NETWORK_H__
#define __NETWORK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/netif.h"
#include "lwip/ip4_addr.h"

// Hardware address meant to match netif hardware address.
typedef struct hwaddr_s
{
  uint8_t hwaddr_len;
  uint8_t hwaddr[NETIF_MAX_HWADDR_LEN];
} hwaddr_t;

void network_init(void);
void network_deinit(void);
bool network_is_up(void);
bool network_use_dhcp(void);
hwaddr_t network_get_hwaddr(void);
ip4_addr_t network_get_address(void);
ip4_addr_t network_get_netmask(void);
ip4_addr_t network_get_gateway(void);
const char *network_get_hostname(void);
ip4_addr_t network_str_to_address(const char *addr_str);
hwaddr_t network_str_to_hwaddr(const char *hwaddr_str);

// System configurable functions to be used for network configuration.
// Implemented as weak functions.
bool network_config_use_dhcp(void);
ip4_addr_t network_config_address(void);
ip4_addr_t network_config_netmask(void);
ip4_addr_t network_config_gateway(void);
hwaddr_t network_config_hwaddr(void);
const char *network_config_hostname(void);
#if LWIP_PTPD
bool network_config_ptpd_slave_only(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NETWORK_H__ */
