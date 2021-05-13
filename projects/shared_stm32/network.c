#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include "cmsis_os2.h"
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/dhcp.h"
#include "lwip/icmp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"
#include "lwip/apps/mdns.h"
#include "lwip/prot/ip4.h"
#include "ethernetif.h"
#include "blink.h"
#include "ntime.h"
#include "syslog.h"
#include "shell.h"
#include "network.h"

#if LWIP_PTPD
#include "ptpd.h"
#endif

#if USE_SPIFLASH
#include "spiflash.h"
#endif

#ifdef __GNUC__
// Define certain external string functions here to simplify cross
// compilation between ARMCC and GNUC.
extern size_t strlcpy(char *, const char *, size_t);
extern int strcasecmp(const char *, const char *);
extern int strncasecmp(const char *, const char *, size_t);
#endif // __GNUC__

// Network interface structure.
static struct netif network_interface;

// Don't build ping if LWIP_RAW not configured for use in lwipopts.h.
#if LWIP_RAW
#define PING_ENABLED              1
#else
#dfeine PING_ENABLED              0
#endif

// Should ping be included?
#if PING_ENABLED

// Define ping thread events.
#define PING_EVENT_RECV           1u
#define PING_EVENT_TIMEOUT        2u
#define PING_EVENT_IO             0x40000000u

// Ping receive timeout - in milliseconds.
#ifndef PING_RCV_TIMEOUT
#define PING_RCV_TIMEOUT            4000
#endif

// Ping send delay - in milliseconds.
#ifndef PING_SEND_DELAY
#define PING_SEND_DELAY             1000
#endif

// Ping identifier - must fit on a u16_t.
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

// Ping additional data size to include in the packet.
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

// Ping variables.
static struct raw_pcb *ping_pcb = NULL;
static uint16_t ping_seq_num = 0;
static uint32_t ping_time = 0;

// Ping command mutex.
static osMutexId_t ping_mutex_id = NULL;

// Ping thread id.
static osThreadId_t ping_thread_id = NULL;

// Prepare a echo ICMP request.
static void ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len)
{
  size_t i;
  size_t data_len = len - sizeof(struct icmp_echo_hdr);

  ICMPH_TYPE_SET(iecho, ICMP_ECHO);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;
  iecho->id = PING_ID;
  iecho->seqno = lwip_htons(++ping_seq_num);

  // Fill the additional data buffer with some data.
  for (i = 0; i < data_len; i++)
  {
    ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char) i;
  }

#if CHECKSUM_GEN_ICMP
  // Fill the checksum.
  iecho->chksum = inet_chksum(iecho, len);
#else
  // Checksum filled by hardware on transmit.
  iecho->chksum = 0u;
#endif
}

// Handles callback on the reception of the ping packet.
static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
  struct icmp_echo_hdr *iecho;
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_ASSERT("p != NULL", p != NULL);

  if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) && pbuf_remove_header(p, PBUF_IP_HLEN) == 0)
  {
    iecho = (struct icmp_echo_hdr *)p->payload;

    // Is this a reply to our earlier echo request?
    if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num)))
    {
      // Send the receive event.
      if (ping_thread_id) osThreadFlagsSet(ping_thread_id, PING_EVENT_RECV);

      pbuf_free(p);

      // Eat the packet.
      return 1;
    }

    // Not eaten, restore original packet.
    pbuf_add_header(p, PBUF_IP_HLEN);
  }

  // Don't eat the packet.
  return 0;
}

// Called if the ping timed out.
static void ping_timeout(void *arg)
{
  // Send the timout event.
  if (ping_thread_id) osThreadFlagsSet(ping_thread_id, PING_EVENT_TIMEOUT);
}

// Send the ping packet.
static void ping_send(struct raw_pcb *raw, const ip_addr_t *addr)
{
  // Determine the size of the ping pbuf.
  size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

  // Allocate the ping pbuf.
  struct pbuf *p = pbuf_alloc(PBUF_IP, (u16_t) ping_size, PBUF_RAM);
  if (p)
  {
    if ((p->len == p->tot_len) && (p->next == NULL))
    {
      struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *) p->payload;
      ping_prepare_echo(iecho, (u16_t) ping_size);
      raw_sendto(raw, p, addr);

      ping_time = ntime_get_msecs();
    }

    // Free the ping pbuf.
    pbuf_free(p);
  }
}

// Send pings and wait for a response. Called from the context of a shell thread.
static void ping_send_now(const ip_addr_t *ping_addr, uint32_t count)
{
  // Set the ping thread.
  ping_thread_id = osThreadGetId();

  // Clear any outstanding flags.
  osThreadFlagsClear(0x7fffffff);

  // Reset the ping sequence number.
  ping_seq_num = 0;

  // Lock the lwIP core mutex.
  LOCK_TCPIP_CORE();

  // Create a ping control block.
  ping_pcb = raw_new(IP_PROTO_ICMP);

  // Did we create the PCB?
  if (ping_pcb != NULL)
  {
    raw_recv(ping_pcb, ping_recv, NULL);
    raw_bind(ping_pcb, IP_ADDR_ANY);
    sys_timeout(PING_RCV_TIMEOUT, ping_timeout, ping_pcb);

    // Send the ping.
    ping_send(ping_pcb, ping_addr);
  }

  // Unlock the lwIP core mutex.
  UNLOCK_TCPIP_CORE();

  // Keep going until count is zero.
  while ((ping_pcb != NULL) && (count > 0))
  {
    // Events to wait for.
    uint32_t event = PING_EVENT_RECV | PING_EVENT_TIMEOUT | PING_EVENT_IO;

    // Wait for an IO event.
    event = osThreadFlagsWait(event, osFlagsWaitAny, tick_from_milliseconds(1000));

    // Is this a receive event?
    if ((event & (0x80000000 | PING_EVENT_RECV)) == PING_EVENT_RECV)
    {
      // Cancel the timeout.
      sys_untimeout(ping_timeout, ping_pcb);

      // Decrement the count.
      count -= 1;

      // Determine the ping round-trip time.
      uint32_t trip_time = ntime_get_msecs() - ping_time;

      // Report reception and round-trip time.
      shell_printf("PING: time%s%u ms\n", trip_time < 1 ? "<" : "=", trip_time < 1 ? 1 : trip_time);

      // Issue another ping?
      if (count > 0)
      {
        // Delay to prevent ping floods.
        uint32_t ping_since = ntime_get_msecs() - ping_time;
        if (ping_since < PING_SEND_DELAY) delay_ms(PING_SEND_DELAY - ping_since);

        // Lock the lwIP core mutex.
        LOCK_TCPIP_CORE();

        // Send again.
        ping_send(ping_pcb, ping_addr);
        sys_timeout(PING_RCV_TIMEOUT, ping_timeout, ping_pcb);

        // Unlock the lwIP core mutex.
        UNLOCK_TCPIP_CORE();
      }
    }

    // Is this a timeout event?
    if ((event & (0x80000000 | PING_EVENT_TIMEOUT)) == PING_EVENT_TIMEOUT)
    {
      // Decrement the count.
      count -= 1;

      // Report the timeout.
      shell_printf("PING: timeout\n");

      // Issue another ping?
      if (count > 0)
      {
        // Delay to prevent ping floods.
        uint32_t ping_since = ntime_get_msecs() - ping_time;
        if (ping_since < PING_SEND_DELAY) delay_ms(PING_SEND_DELAY - ping_since);

        // Lock the lwIP core mutex.
        LOCK_TCPIP_CORE();

        // Send again.
        ping_send(ping_pcb, ping_addr);
        sys_timeout(PING_RCV_TIMEOUT, ping_timeout, ping_pcb);

        // Unlock the lwIP core mutex.
        UNLOCK_TCPIP_CORE();
      }
    }

    // Is this an IO event? 
    if ((event & (0x80000000 | PING_EVENT_IO)) == PING_EVENT_IO)
    {
      // Cancel the timeout.
      sys_untimeout(ping_timeout, ping_pcb);

      // Consume the character.
      if (shell_hasc()) shell_getc();

      // Stop by setting the count to zero.
      count = 0;
    }
  }

  // Lock the lwIP core mutex.
  LOCK_TCPIP_CORE();

  // Free the bing control block.
  if (ping_pcb) raw_remove(ping_pcb);
  ping_pcb = NULL;

  // Unlock the lwIP core mutex.
  UNLOCK_TCPIP_CORE();

  // Reset the ping thread id.
  ping_thread_id = NULL;
}

// Ping shell command called from a shell thread.
static bool ping_shell_command(int argc, char **argv)
{
  bool need_help = false;

  // We must be given a ping.
  if (argc > 1)
  {
    // Lock the ping mutex.
    if (osMutexAcquire(ping_mutex_id, 0) == osOK)
    {
      ip4_addr_t address = { IPADDR_ANY };
      uint32_t count = 4;

      // Get the ping address.
      ip4addr_aton(argv[1], &address);

      // Do we have a count?
      if (argc > 2)
      {
        // Get the count.
        count = (uint32_t) strtol(argv[2], NULL, 0);

        // Keep counts reasonable.
        if (count > 100) count = 100;
      }

      // Perform the ping.
      ping_send_now(&address, count);

      // Release the ping mutex.
      osMutexRelease(ping_mutex_id);
    }
    else
    {
      // Ping is busy.
      shell_puts("PING: busy\n");
    }
  }
  else
  {
    // We need help.
    need_help = true;
  }

  // Do we need to display help?
  if (need_help)
  {
    shell_puts("Usage:\n");
    shell_printf("  %s addr [count]\n", argv[0]);
  }

  return true;
}

// Initialize the ping module.
static void ping_init(void)
{
  // Static control blocks.
  static uint32_t ping_mutex_cb[osRtxMutexCbSize/4U] __attribute__((section(".bss.os.mutex.cb")));

  // Ping mutex attributes. Note the mutex is not recursive.
  osMutexAttr_t mutex_attrs =
  {
    .name = "ping",
    .attr_bits = 0u,
    .cb_mem = ping_mutex_cb,
    .cb_size = sizeof(ping_mutex_cb)
  };

  // Create the ping mutex.
  ping_mutex_id = osMutexNew(&mutex_attrs);
  if (ping_mutex_id == NULL)
  {
    // Report an error.
    syslog_printf(SYSLOG_ERROR, "PING: cannot create mutex");
  }

  // Add the shell command.
  shell_add_command("ping", ping_shell_command);
}

#endif /* PING_ENABLED */

// Report the current status of the ethernet port.
static void network_address_dump(struct netif *netif)
{
  ip4_addr_t ipaddr = netif->ip_addr;
  ip4_addr_t netmask = netif->netmask;
  ip4_addr_t gateway = netif->gw;
  syslog_printf(SYSLOG_INFO, "NETWORK: address is %d.%d.%d.%d",
                (uint8_t) ipaddr.addr, (uint8_t) (ipaddr.addr >> 8), 
                (uint8_t) (ipaddr.addr >> 16), (uint8_t) (ipaddr.addr >> 24));
  syslog_printf(SYSLOG_INFO, "NETWORK: netmask is %d.%d.%d.%d",
                (uint8_t) netmask.addr, (uint8_t) (netmask.addr >> 8), 
                (uint8_t) (netmask.addr >> 16), (uint8_t) (netmask.addr >> 24));
  syslog_printf(SYSLOG_INFO, "NETWORK: gateway is %d.%d.%d.%d",
                (uint8_t) gateway.addr, (uint8_t) (gateway.addr >> 8), 
                (uint8_t) (gateway.addr >> 16), (uint8_t) (gateway.addr >> 24));
}

// Callback called when link is brought up/down. This callback is tied to the
// function netif_is_link_up() returning true when link is up.
static void network_link_callback(struct netif *netif)
{
  bool network_link_is_up = netif_is_link_up(&network_interface);

  // Report if the network link is up or down.
  syslog_printf(SYSLOG_INFO, "NETWORK: link is %s", network_link_is_up ? "up" : "down");

  // Is the network up or down?
  if (network_link_is_up)
  {
    // Link is up. Blink at a slow rate.
    blink_set_rate(2);

    // Should we attempt DHCP configuration?
    if (network_use_dhcp())
    {
      // Start or restart DHCP negotiation for this network interface.
      // If LWIP_DHCP_AUTOIP_COOP is defined, DHCP and AUTOIP will both 
      // be both enabled at the same time.
      dhcp_start(&network_interface);
    }

#if !LWIP_NETIF_EXT_STATUS_CALLBACK
    // The following is not automatically called if LWIP_NETIF_EXT_STATUS_CALLBACK is not defined.
    mdns_resp_netif_settings_changed(&network_interface);
#endif
  }
  else
  {
    // Blink at a fast rate.
    blink_set_rate(10);
  }
}

// Callback called when interface is brought up/down or address is changed while up.
// This callback is tied to the function netif_is_up() returning true when link is up.
static void network_status_callback(struct netif *netif)
{
  // Is the network interface is up?
  if (netif_is_up(&network_interface))
  {
    // Dump the static IP addresses.
    network_address_dump(&network_interface);
  }
}

#if LWIP_STATS
static void network_stats_proto(struct stats_proto *proto, const char *name)
{
  shell_printf("\n%s\n", name);
  shell_printf("  xmit: %"STAT_COUNTER_F"\n", proto->xmit);
  shell_printf("  recv: %"STAT_COUNTER_F"\n", proto->recv);
  shell_printf("  fw: %"STAT_COUNTER_F"\n", proto->fw);
  shell_printf("  drop: %"STAT_COUNTER_F"\n", proto->drop);
  shell_printf("  chkerr: %"STAT_COUNTER_F"\n", proto->chkerr);
  shell_printf("  lenerr: %"STAT_COUNTER_F"\n", proto->lenerr);
  shell_printf("  memerr: %"STAT_COUNTER_F"\n", proto->memerr);
  shell_printf("  rterr: %"STAT_COUNTER_F"\n", proto->rterr);
  shell_printf("  proterr: %"STAT_COUNTER_F"\n", proto->proterr);
  shell_printf("  opterr: %"STAT_COUNTER_F"\n", proto->opterr);
  shell_printf("  err: %"STAT_COUNTER_F"\n", proto->err);
  shell_printf("  cachehit: %"STAT_COUNTER_F"\n", proto->cachehit);
}

#if IGMP_STATS || MLD6_STATS
static void network_stats_igmp(struct stats_igmp *igmp, const char *name)
{
  shell_printf("\n%s\n", name);
  shell_printf("  xmit: %"STAT_COUNTER_F"\n", igmp->xmit);
  shell_printf("  recv: %"STAT_COUNTER_F"\n", igmp->recv);
  shell_printf("  drop: %"STAT_COUNTER_F"\n", igmp->drop);
  shell_printf("  chkerr: %"STAT_COUNTER_F"\n", igmp->chkerr);
  shell_printf("  lenerr: %"STAT_COUNTER_F"\n", igmp->lenerr);
  shell_printf("  memerr: %"STAT_COUNTER_F"\n", igmp->memerr);
  shell_printf("  proterr: %"STAT_COUNTER_F"\n", igmp->proterr);
  shell_printf("  rx_v1: %"STAT_COUNTER_F"\n", igmp->rx_v1);
  shell_printf("  rx_group: %"STAT_COUNTER_F"\n", igmp->rx_group);
  shell_printf("  rx_general: %"STAT_COUNTER_F"\n", igmp->rx_general);
  shell_printf("  rx_report: %"STAT_COUNTER_F"\n", igmp->rx_report);
  shell_printf("  tx_join: %"STAT_COUNTER_F"\n", igmp->tx_join);
  shell_printf("  tx_leave: %"STAT_COUNTER_F"\n", igmp->tx_leave);
  shell_printf("  tx_report: %"STAT_COUNTER_F"\n", igmp->tx_report);
}
#endif

#if MEM_STATS || MEMP_STATS
static void network_stats_mem(struct stats_mem *mem, const char *name)
{
  shell_printf("\nMEM %s\n", name);
  shell_printf("  avail: %"U32_F"\n", (u32_t) mem->avail);
  shell_printf("  used: %"U32_F"\n", (u32_t) mem->used);
  shell_printf("  max: %"U32_F"\n", (u32_t) mem->max);
  shell_printf("  err: %"U32_F"\n", (u32_t) mem->err);
}

#if MEMP_STATS
static void network_stats_memp(struct stats_mem *mem, int index)
{
  if (index < MEMP_MAX) {
    network_stats_mem(mem, mem->name);
  }
}
#endif
#endif
#endif

// Network shell command.
static bool network_shell_network(int argc, char **argv)
{
#if LWIP_STATS
  // Are we given an argument?
  if (argc == 2)
  {
    // Determine the stats to print.
    if (!strcasecmp("all", argv[1]))
    {
#if LINK_STATS
      network_stats_proto(&lwip_stats.link, "LINK");
#endif
#if IP_STATS
      network_stats_proto(&lwip_stats.ip, "IP");
#endif
#if TCP_STATS
      network_stats_proto(&lwip_stats.tcp, "TCP");
#endif
#if UDP_STATS
      network_stats_proto(&lwip_stats.udp, "UDP");
#endif
#if ICMP_STATS
      network_stats_proto(&lwip_stats.icmp, "ICMP");
#endif
#if IPFRAG_STATS
      network_stats_proto(&lwip_stats.ip_frag, "IP_FRAG");
#endif
#if ETHARP_STATS
      network_stats_proto(&lwip_stats.etharp, "ETHARP");
#endif
#if IGMP_STATS
      network_stats_igmp(&lwip_stats.igmp, "IGMP");
#endif
#if MEM_STATS
      network_stats_mem(&lwip_stats.mem, "HEAP");
#if MEMP_STATS
      for (int i = 0; i < MEMP_MAX; i++)
      {
        network_stats_memp(lwip_stats.memp[i], i);
      }
#endif
#endif
    }
#if IP_STATS
    else if (!strcasecmp("ip", argv[1]))
    {
      network_stats_proto(&lwip_stats.ip, "IP");
    }
#endif
#if TCP_STATS
    else if (!strcasecmp("tcp", argv[1]))
    {
      network_stats_proto(&lwip_stats.tcp, "TCP");
    }
#endif
#if UDP_STATS
    else if (!strcasecmp("udp", argv[1]))
    {
      network_stats_proto(&lwip_stats.udp, "UDP");
    }
#endif
#if ICMP_STATS
    else if (!strcasecmp("icmp", argv[1]))
    {
      network_stats_proto(&lwip_stats.icmp, "ICMP");
    }
#endif
#if LINK_STATS
    else if (!strcasecmp("link", argv[1]))
    {
      network_stats_proto(&lwip_stats.link, "LINK");
    }
#endif
#if IPFRAG_STATS
    else if (!strcasecmp("ipfrag", argv[1]))
    {
      network_stats_proto(&lwip_stats.ip_frag, "IP_FRAG");
    }
#endif
#if ETHARP_STATS
    else if (!strcasecmp("etharp", argv[1]))
    {
      network_stats_proto(&lwip_stats.etharp, "ETHARP");
    }
#endif
#if IGMP_STATS
    else if (!strcasecmp("igmp", argv[1]))
    {
      network_stats_igmp(&lwip_stats.igmp, "IGMP");
    }
#endif
#if MEM_STATS
    else if (!strcasecmp("mem", argv[1]))
    {
      network_stats_mem(&lwip_stats.mem, "HEAP");
#if MEMP_STATS
      for (int i = 0; i < MEMP_MAX; i++)
      {
        network_stats_memp(lwip_stats.memp[i], i);
      }
#endif
    }
#endif
    else
    {
      shell_puts("Usage:\n");
      shell_puts("    network all\n");
#if IP_STATS
      shell_puts("    network ip\n");
#endif
#if TCP_STATS
      shell_puts("    network tcp\n");
#endif
#if UDP_STATS
      shell_puts("    network udp\n");
#endif
#if ICMP_STATS
      shell_puts("    network icmp\n");
#endif
#if LINK_STATS
      shell_puts("    network link\n");
#endif
#if IPFRAG_STATS
      shell_puts("    network ipfrag\n");
#endif
#if ETHARP_STATS
      shell_puts("    network etharp\n");
#endif
#if IGMP_STATS
      shell_puts("    network igmp\n");
#endif
#if MEM_STATS
      shell_puts("    network mem\n");
#endif
    }
  }
  else
#endif
  {
    uint8_t i;

    // Get the network addresses in use.
    ip4_addr_t address = network_interface.ip_addr;
    ip4_addr_t netmask = network_interface.netmask;
    ip4_addr_t gateway = network_interface.gw;

    // Print the network information.
    shell_printf("hostname: %s\n", network_get_hostname());
    shell_printf("dhcp: %s\n", network_use_dhcp() ? "enabled" : "disabled");
    shell_printf("address: %d.%d.%d.%d\n",
                 (uint8_t) address.addr, (uint8_t) (address.addr >> 8), 
                 (uint8_t) (address.addr >> 16), (uint8_t) (address.addr >> 24));
    shell_printf("netmask: %d.%d.%d.%d\n",
                 (uint8_t) netmask.addr, (uint8_t) (netmask.addr >> 8), 
                 (uint8_t) (netmask.addr >> 16), (uint8_t) (netmask.addr >> 24));
    shell_printf("gateway: %d.%d.%d.%d\n",
                 (uint8_t) gateway.addr, (uint8_t) (gateway.addr >> 8), 
                 (uint8_t) (gateway.addr >> 16), (uint8_t) (gateway.addr >> 24));
    shell_puts("hwaddr: ");
    for (i = 0; i < network_interface.hwaddr_len; ++i)
    {
      shell_printf("%02x%s", (int) network_interface.hwaddr[i], i < (network_interface.hwaddr_len - 1) ? ":" : "\n");
    }

    // Print the network interface counts.
    uint32_t recv_count, recv_bytes, send_count, send_bytes;
    ethernetif_counts(&recv_count, &recv_bytes, &send_count, &send_bytes);
    shell_printf("recv: %u (%u bytes)\n", recv_count, recv_bytes);
    shell_printf("send: %u (%u bytes)\n", send_count, send_bytes);
  }

  return true;
}

// Initializes Ethernet networking.
void network_init(void)
{
#if USE_SPIFLASH
  // Parse the configuration file.
  if (!spiflash_config_parse("network.conf", network_config_parser))
  {
    syslog_printf(SYSLOG_ERROR, "NETWORK: failed to open 'network.conf' config file");
  }
#endif

  // Initialize the LWIP module and create the LWIP TCP/IP thread.
  tcpip_init(NULL, NULL);

  // Lock the lwIP core mutex.
  LOCK_TCPIP_CORE();

  ip4_addr_t address = network_config_address();
  ip4_addr_t netmask = network_config_netmask();
  ip4_addr_t gateway = network_config_gateway();

  // Add the network interface to the netif_list. Allocate a struct netif and pass a pointer
  // to this structure as the first argument. Give pointers to cleared ip_addr structures
  // when using DHCP, or fill them with sane numbers otherwise. The state pointer may be NULL.
  netif_add(&network_interface, &address, &netmask, &gateway, NULL, &ethernetif_init, &tcpip_input);

  // Registers the default network interface.
  netif_set_default(&network_interface);

  // Initialize Multicast-DNS (MDNS) responder.
  mdns_resp_init();

  // Configure Multicast-DNS (MDNS) responder.
  mdns_resp_add_netif(&network_interface, network_get_hostname(), 60);

  // Set callback to be called when link is brought up/down.
  netif_set_link_callback(&network_interface, network_link_callback);

  // Set callback to be called when interface is brought up/down or address is changed while up.
  netif_set_status_callback(&network_interface, network_status_callback);

  // Unlock the lwIP core mutex.
  UNLOCK_TCPIP_CORE();

#if LWIP_PTPD
    // Initialize PTPD daemon in slave only or master/slave mode.
    ptpd_init(network_config_ptpd_slave_only());
#endif

  // Add the shell command.
  shell_add_command("network", network_shell_network);

#if PING_ENABLED
  // Initialize the ping command.
  ping_init();
#endif
}

// De-initialize Ethernet networking.
void network_deinit(void)
{
  // De-initialize the network interface.
  ethernetif_deinit(&network_interface);
}

// Return true if the network is up.
bool network_is_up(void)
{
  // Lock the lwIP core mutex.
  LOCK_TCPIP_CORE();

  // Query if the network interface is up.
  bool is_up = netif_is_up(&network_interface) ? true : false;

  // Unlock the lwIP core mutex.
  UNLOCK_TCPIP_CORE();

  return is_up;
}

// Get the configured use DHCP flag.
bool network_use_dhcp(void)
{
  // Use the system configured DHCP flag.
  return network_config_use_dhcp();
}

// Get the network interface hardware address.
hwaddr_t network_get_hwaddr(void)
{
  // Return the configured hardware address.
  hwaddr_t hwaddr;
  hwaddr.hwaddr_len = ETH_HWADDR_LEN;
  hwaddr.hwaddr[0] = network_interface.hwaddr[0];
  hwaddr.hwaddr[1] = network_interface.hwaddr[1];
  hwaddr.hwaddr[2] = network_interface.hwaddr[2];
  hwaddr.hwaddr[3] = network_interface.hwaddr[3];
  hwaddr.hwaddr[4] = network_interface.hwaddr[4];
  hwaddr.hwaddr[5] = network_interface.hwaddr[5];
  return hwaddr;
}

// Get the network interface network IP address.
ip4_addr_t network_get_address(void)
{
  // Get the network IP address from the network interface.
  return network_interface.ip_addr;
}

// Get the network interface netmask.
ip4_addr_t network_get_netmask(void)
{
  // Get the netmask from the network interface.
  return network_interface.netmask;
}

// Get the network interface gateway IP address.
ip4_addr_t network_get_gateway(void)
{
  // Get the gateway IP address from the network interface.
  return network_interface.gw;
}

// Get the configured hostname.
const char *network_get_hostname(void)
{
  // Use the system configured hostname.
  return network_config_hostname();
}

// Converts a string to an IP4 address.
ip4_addr_t network_str_to_address(const char *addr_str)
{
  ip4_addr_t addr;

  // Convert the string to an IP4 address.
  if (!addr_str || !ip4addr_aton(addr_str, &addr))
  {
    // Ooops.  Return the ANY (all zero) address.
    ip4_addr_set_u32(&addr, IPADDR_ANY);
  }

  return addr;
}

// Converts a string to a MAC hardware address.
hwaddr_t network_str_to_hwaddr(const char *hwaddr_str)
{
  hwaddr_t hwaddr;
  uint8_t *pp = (uint8_t *) hwaddr.hwaddr;

  // Set the length.
  hwaddr.hwaddr_len = sizeof(hwaddr.hwaddr);
  memset(&hwaddr.hwaddr, 0, sizeof(hwaddr.hwaddr));

  // Sanity check the string.
  if (!hwaddr_str) return hwaddr;

  // Get the first character.
  char c = *hwaddr_str;
  for (;;)
  {
    // Collect hex number up to ':'.
    uint32_t val = 0;
    for (;;)
    {
      if (isdigit(c))
      {
        val = (val << 4) + (uint32_t) (c - '0');
        c = *++hwaddr_str;
      }
      else if (isxdigit(c))
      {
        val = (val << 4) | (uint32_t) (c + 10 - (islower(c) ? 'a' : 'A'));
        c = *++hwaddr_str;
      }
      else
      {
        break;
      }
    }
    if (c == ':')
    {
      *pp++ = (int8_t) val;
      c = *++hwaddr_str;
    }
    else
    {
      *pp++ = (int8_t) val;
      break;
    }
    if (pp >= hwaddr.hwaddr + sizeof(hwaddr.hwaddr))
    {
      break;
    }
  }

  // Check for incomplete parsing.
  if (pp != hwaddr.hwaddr + sizeof(hwaddr.hwaddr))
  {
    // Reset the address to all zeros.
    memset(&hwaddr.hwaddr, 0, sizeof(hwaddr.hwaddr));
  }

  return hwaddr;
}

// System configurable DHCP flag.
__WEAK bool network_config_use_dhcp(void)
{
  // Enable DHCP by default.
  return true;
}

// System configurable network IP address.
__WEAK ip4_addr_t network_config_address(void)
{
  // Default network address.
  ip4_addr_t addr = { IPADDR_ANY };
  return addr;
}

// System configurable netmask.
__WEAK ip4_addr_t network_config_netmask(void)
{
  // Default network netmask.
  ip4_addr_t addr = { IPADDR_ANY };
  return addr;
}

// System configurable gateway IP address.
__WEAK ip4_addr_t network_config_gateway(void)
{
  // Default network gateway.
  ip4_addr_t addr = { IPADDR_ANY };
  return addr;
}

// System configurable hardware address.
__WEAK hwaddr_t network_config_hwaddr(void)
{
  // Default network hardware address.
  hwaddr_t hwaddr = { 6, { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00 } };
  return hwaddr;
}

// System configurable hostname.
__WEAK const char *network_config_hostname(void)
{
  // Default hostname.
  return "STM32";
}

#if LWIP_PTPD
// System configurable PTPD flag.
__WEAK bool network_config_ptpd_slave_only(void)
{
  // Default PTPD as slave only.
  return true;
}
#endif
