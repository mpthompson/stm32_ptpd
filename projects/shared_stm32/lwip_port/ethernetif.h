#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "lwip/err.h"
#include "lwip/netif.h"
#include "cmsis_os2.h"
#include "ethptp.h"

// Ethernet interface lwIP initialization and de-initialization functions.
err_t ethernetif_init(struct netif *netif);
err_t ethernetif_deinit(struct netif *netif);

// Ethernet interface count functions.
void ethernetif_counts(uint32_t *recv_count, uint32_t *recv_bytes, uint32_t *send_count, uint32_t *send_bytes);

#if LWIP_PTPD
void ethernetif_get_tx_timestamp(struct pbuf *p);
#endif

void ethernetif_ptp_start(uint32_t update_method);
void ethernetif_ptp_get_time(ptptime_t *timestamp);
void ethernetif_ptp_set_time(ptptime_t *timestamp);
void ethernetif_ptp_update_offset(ptptime_t *timestamp);
void ethernetif_ptp_adj_freq(int32_t adj_ppb);

// System configurable functions. Implemented as weak functions.
uint32_t ethernetif_config_preempt_priority(void);

#endif
