#ifndef __LWIPOPTS_SHARED_H__
#define __LWIPOPTS_SHARED_H__

#include "cmsis_os2.h"
#include "delay.h"
#include "random.h"

/* Use random_get() as platform random number function. */
#define LWIP_RAND                       random_get

/* System millisecond delay function. */
#define sys_msleep                      delay_ms

// Have lwIP provide errno.
#ifndef LWIP_PROVIDE_ERRNO
#define LWIP_PROVIDE_ERRNO              1
#endif

/* To use the struct timeval provided system, set this to 0 and
 * include <sys/time.h> in cc.h. */
#ifndef LWIP_TIMEVAL_PRIVATE
#if !defined(__ARMCC_VERSION) && defined (__GNUC__)
#define LWIP_TIMEVAL_PRIVATE            0
#else
#define LWIP_TIMEVAL_PRIVATE            1
#endif
#endif

/* -------------- NO SYS -------------- */

/* NO_SYS==1: Use lwIP without OS-awareness (no thread, semaphores, mutexes or
 * mboxes). With an RTOS this should not be set. */
#if !defined NO_SYS
#define NO_SYS                          0
#endif

/* LWIP_TIMERS==0: Drop support for sys_timeout and lwip-internal cyclic timers. */
#if !defined LWIP_TIMERS
#define LWIP_TIMERS                     1
#endif

/* LWIP_TIMERS_CUSTOM==1: Provide your own timer implementation.
 * Function prototypes in timeouts.h and the array of lwip-internal cyclic timers
 * are still included, but the implementation is not. The following functions
 * will be required: sys_timeouts_init(), sys_timeout(), sys_untimeout(),
 *                   sys_timeouts_mbox_fetch() */
#if !defined LWIP_TIMERS_CUSTOM
#define LWIP_TIMERS_CUSTOM              0
#endif

/* ---------- Memory copying ---------- */

/*MEMCPY: override this if you have a faster implementation at hand than the
 * one included in your C library */
#if !defined MEMCPY
#define MEMCPY(dst,src,len)             memcpy(dst,src,len)
#endif

/* SMEMCPY: override this with care! Some compilers (e.g. gcc) can inline a
 * call to memcpy() if the length is known at compile time and is small. */
#if !defined SMEMCPY
#define SMEMCPY(dst,src,len)            memcpy(dst,src,len)
#endif

/* MEMMOVE: override this if you have a faster implementation at hand than the
 * one included in your C library.  lwIP currently uses MEMMOVE only when IPv6
 * fragmentation support is enabled. */
#if !defined MEMMOVE
#define MEMMOVE(dst,src,len)            memmove(dst,src,len)
#endif

/* ----------- Core locking ----------- */

/* LWIP_MPU_COMPATIBLE: enables special memory management mechanism
 * which makes lwip able to work on MPU (Memory Protection Unit) system
 * by not passing stack-pointers to other threads
 * (this decreases performance as memory is allocated from pools instead
 * of keeping it on the stack) */
#if !defined LWIP_MPU_COMPATIBLE
#define LWIP_MPU_COMPATIBLE             0
#endif

/* LWIP_TCPIP_CORE_LOCKING
 * Creates a global mutex that is held during TCPIP thread operations.
 * Can be locked by client code to perform lwIP operations without changing
 * into TCPIP thread using callbacks. See LOCK_TCPIP_CORE() and
 * UNLOCK_TCPIP_CORE().
 * Your system should provide mutexes supporting priority inversion to
 * use this. */
#if !defined LWIP_TCPIP_CORE_LOCKING
#define LWIP_TCPIP_CORE_LOCKING         1
#endif

/* LWIP_TCPIP_CORE_LOCKING_INPUT: when LWIP_TCPIP_CORE_LOCKING is enabled,
 * this lets tcpip_input() grab the mutex for input packets as well,
 * instead of allocating a message and passing it to tcpip_thread.
 * ATTENTION: this does not work when tcpip_input() is called from
 * interrupt context! */
#if !defined LWIP_TCPIP_CORE_LOCKING_INPUT
#define LWIP_TCPIP_CORE_LOCKING_INPUT   1
#endif

/* SYS_LIGHTWEIGHT_PROT==1: if you want inter-task protection for certain
 * critical regions during buffer allocation, deallocation and memory
 * allocation and deallocation. */
#if !defined SYS_LIGHTWEIGHT_PROT
#define SYS_LIGHTWEIGHT_PROT            1
#endif

/* Macro/function to check whether lwIP's threading/locking
 * requirements are satisfied during current function call.
 * This macro usually calls a function that is implemented in the OS-dependent
 * sys layer and performs the following checks:
 * - Not in ISR (this should be checked for NO_SYS==1, too!)
 * - If @ref LWIP_TCPIP_CORE_LOCKING = 1: TCPIP core lock is held
 * - If @ref LWIP_TCPIP_CORE_LOCKING = 0: function is called from TCPIP thread */
#if !defined LWIP_ASSERT_CORE_LOCKED
#define LWIP_ASSERT_CORE_LOCKED()
#endif

/* Called as first thing in the lwIP TCPIP thread. Can be used in conjunction
 * with @ref LWIP_ASSERT_CORE_LOCKED to check core locking. */
#if !defined LWIP_MARK_TCPIP_THREAD
#define LWIP_MARK_TCPIP_THREAD()
#endif

/* ---------- Memory options ---------- */

/* MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by your C-library
 * instead of the lwip internal allocator. Can save code size if you
 * already use it. */
#if !defined MEM_LIBC_MALLOC
#define MEM_LIBC_MALLOC                 0
#endif

/* MEMP_MEM_MALLOC==1: Use mem_malloc/mem_free instead of the lwip pool allocator.
 * Especially useful with MEM_LIBC_MALLOC but handle with care regarding execution
 * speed and usage from interrupts! */
#if !defined MEMP_MEM_MALLOC
#define MEMP_MEM_MALLOC                 0
#endif

/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
 * lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
 * byte alignment -> define MEM_ALIGNMENT to 2. */
#if !defined MEM_ALIGNMENT
#define MEM_ALIGNMENT                   4
#endif

/* MEM_SIZE: the size of the heap memory. If the application will send
 * a lot of data that needs to be copied, this should be set high. */
#if !defined MEM_SIZE
#define MEM_SIZE                        (4 * 1024)
#endif

/* MEMP_OVERFLOW_CHECK: memp overflow protection reserves a configurable
 * amount of bytes before and after each memp element in every pool and fills
 * it with a prominent default value.
 *    MEMP_OVERFLOW_CHECK == 0 no checking
 *    MEMP_OVERFLOW_CHECK == 1 checks each element when it is freed
 *    MEMP_OVERFLOW_CHECK >= 2 checks each element in every pool every time
 *      memp_malloc() or memp_free() is called (useful but slow!) */
#if !defined MEMP_OVERFLOW_CHECK
#define MEMP_OVERFLOW_CHECK             0
#endif

/* MEMP_SANITY_CHECK==1: run a sanity check after each memp_free() to make
 * sure that there are no cycles in the linked lists. Enabling this option 
 * can negatively impact performance. */
#if !defined MEMP_SANITY_CHECK
#define MEMP_SANITY_CHECK               0
#endif

/* MEM_USE_POOLS==1: Use an alternative to malloc() by allocating from a set
 * of memory pools of various sizes. When mem_malloc is called, an element of
 * the smallest pool that can provide the length needed is returned.
 * To use this, MEMP_USE_CUSTOM_POOLS also has to be enabled. */
#if !defined MEM_USE_POOLS
#define MEM_USE_POOLS                   0
#endif

/* MEM_USE_POOLS_TRY_BIGGER_POOL==1: if one malloc-pool is empty, try the next
 * bigger pool - WARNING: THIS MIGHT WASTE MEMORY but it can make a system more
 * reliable. */
#if !defined MEM_USE_POOLS_TRY_BIGGER_POOL
#define MEM_USE_POOLS_TRY_BIGGER_POOL   0
#endif

/* MEMP_USE_CUSTOM_POOLS==1: whether to include a user file lwippools.h
 * that defines additional pools beyond the "standard" ones required
 * by lwIP. If you set this to 1, you must have lwippools.h in your
 * include path somewhere. */
#if !defined MEMP_USE_CUSTOM_POOLS
#define MEMP_USE_CUSTOM_POOLS           0
#endif

/* Set this to 1 if you want to free PBUF_RAM pbufs (or call mem_free()) from
 * interrupt context (or another context that doesn't allow waiting for a
 * semaphore).
 * If set to 1, mem_malloc will be protected by a semaphore and SYS_ARCH_PROTECT,
 * while mem_free will only use SYS_ARCH_PROTECT. mem_malloc SYS_ARCH_UNPROTECTs
 * with each loop so that mem_free can run.
 *
 * ATTENTION: As you can see from the above description, this leads to dis-/
 * enabling interrupts often, which can be slow! Also, on low memory, mem_malloc
 * can need longer.
 *
 * If you don't want that, at least for NO_SYS=0, you can still use the following
 * functions to enqueue a deallocation call which then runs in the tcpip_thread
 * context:
 * - pbuf_free_callback(p);
 * - mem_free_callback(m); */
#if !defined LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 0
#endif

/* ---------- Internal Memory Pool Sizes ---------- */

/* MEMP_NUM_PBUF: the number of memp struct pbufs (used for PBUF_ROM and PBUF_REF).
 * If the application sends a lot of data out of ROM (or other static memory),
 * this should be set high. */
#if !defined MEMP_NUM_PBUF
#define MEMP_NUM_PBUF                   16
#endif

/* MEMP_NUM_RAW_PCB: Number of raw connection PCBs
 * (requires the LWIP_RAW option) */
#if !defined MEMP_NUM_RAW_PCB
#define MEMP_NUM_RAW_PCB                4
#endif

/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
 * per active UDP "connection". (requires the LWIP_UDP option) */
#if !defined MEMP_NUM_UDP_PCB
#define MEMP_NUM_UDP_PCB                8
#endif

/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP connections.
 * (requires the LWIP_TCP option) */
#if !defined MEMP_NUM_TCP_PCB
#define MEMP_NUM_TCP_PCB                5
#endif

/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP connections.
 * (requires the LWIP_TCP option) */
#if !defined MEMP_NUM_TCP_PCB_LISTEN
#define MEMP_NUM_TCP_PCB_LISTEN         8
#endif

/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP segments.
 * (requires the LWIP_TCP option) */
#if !defined MEMP_NUM_TCP_SEG
#define MEMP_NUM_TCP_SEG                16
#endif

/* MEMP_NUM_ALTCP_PCB: the number of simultaneously active altcp layer pcbs.
 * (requires the LWIP_ALTCP option)
 * Connections with multiple layers require more than one altcp_pcb (e.g. TLS
 * over TCP requires 2 altcp_pcbs, one for TLS and one for TCP). */
#if !defined MEMP_NUM_ALTCP_PCB
#define MEMP_NUM_ALTCP_PCB              MEMP_NUM_TCP_PCB
#endif

/* MEMP_NUM_REASSDATA: the number of simultaneously IP packets queued for
 * reassembly (whole packets, not fragments!) */
#if !defined MEMP_NUM_REASSDATA
#define MEMP_NUM_REASSDATA              5
#endif

/* MEMP_NUM_FRAG_PBUF: the number of IP fragments simultaneously sent
 * (fragments, not whole packets!).
 * This is only used with LWIP_NETIF_TX_SINGLE_PBUF==0 and only has to be > 1
 * with DMA-enabled MACs where the packet is not yet sent when netif->output
 * returns. */
#if !defined MEMP_NUM_FRAG_PBUF
#define MEMP_NUM_FRAG_PBUF              15
#endif

/* MEMP_NUM_ARP_QUEUE: the number of simulateously queued outgoing
 * packets (pbufs) that are waiting for an ARP request (to resolve
 * their destination address) to finish. (requires the ARP_QUEUEING 
 * option) */
#if !defined MEMP_NUM_ARP_QUEUE
#define MEMP_NUM_ARP_QUEUE              30
#endif

/* MEMP_NUM_IGMP_GROUP: The number of multicast groups whose network interfaces
 * can be members at the same time (one per netif - allsystems group -, plus one
 * per netif membership). (requires the LWIP_IGMP option) */
#if !defined MEMP_NUM_IGMP_GROUP
#define MEMP_NUM_IGMP_GROUP             8
#endif

/* The number of sys timeouts used by the core stack (not apps)
 * The default number of timeouts is calculated here for all enabled modules. */
#define LWIP_NUM_SYS_TIMEOUT_LOCAL     (LWIP_TCP + IP_REASSEMBLY + LWIP_ARP + (2*LWIP_DHCP) + LWIP_AUTOIP + LWIP_IGMP + LWIP_DNS + PPP_NUM_TIMEOUTS + (LWIP_IPV6 * (1 + LWIP_IPV6_REASS + LWIP_IPV6_MLD)))

/* MEMP_NUM_SYS_TIMEOUT: the number of simultaneously active timeouts.
 * We add two addition for MDNS application. */
#if !defined MEMP_NUM_SYS_TIMEOUT
#define MEMP_NUM_SYS_TIMEOUT            LWIP_NUM_SYS_TIMEOUT_LOCAL + 2
#endif

/* MEMP_NUM_NETBUF: the number of struct netbufs.
 * (only needed if you use the sequential API, like api_lib.c) */
#if !defined MEMP_NUM_NETBUF
#define MEMP_NUM_NETBUF                 2
#endif

/* MEMP_NUM_NETCONN: the number of struct netconns.  MEMP_NUM_NETCONN should be 
 * less than the sum of MEMP_NUM_{TCP,RAW,UDP}_PCB+MEMP_NUM_TCP_PCB_LISTEN
 * (only needed if you use the sequential API, like api_lib.c) */
#if !defined MEMP_NUM_NETCONN
#define MEMP_NUM_NETCONN                4
#endif

/* MEMP_NUM_SELECT_CB: the number of struct lwip_select_cb.
 * (Only needed if you have LWIP_MPU_COMPATIBLE==1 and use the socket API.
 * In that case, you need one per thread calling lwip_select.) */
#if !defined MEMP_NUM_SELECT_CB
#define MEMP_NUM_SELECT_CB              4
#endif

/* MEMP_NUM_TCPIP_MSG_API: the number of struct tcpip_msg, which are used
 * for callback/timeout API communication. (only needed if you use tcpip.c) */
#if !defined MEMP_NUM_TCPIP_MSG_API
#define MEMP_NUM_TCPIP_MSG_API          8
#endif

/* MEMP_NUM_TCPIP_MSG_INPKT: the number of struct tcpip_msg, which are used
 * for incoming packets. (only needed if you use tcpip.c) */
#if !defined MEMP_NUM_TCPIP_MSG_INPKT
#define MEMP_NUM_TCPIP_MSG_INPKT        8
#endif

/* MEMP_NUM_NETDB: the number of concurrently running lwip_addrinfo() calls
 * (before freeing the corresponding memory using lwip_freeaddrinfo()). */
#if !defined MEMP_NUM_NETDB
#define MEMP_NUM_NETDB                  1
#endif

/* MEMP_NUM_LOCALHOSTLIST: the number of host entries in the local host list
 * if DNS_LOCAL_HOSTLIST_IS_DYNAMIC==1. */
#if !defined MEMP_NUM_LOCALHOSTLIST
#define MEMP_NUM_LOCALHOSTLIST          1
#endif

/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#if !defined PBUF_POOL_SIZE
#define PBUF_POOL_SIZE                  16
#endif

/* ---------- ARP options ---------- */

/* LWIP_ARP==1: Enable ARP functionality. */
#if !defined LWIP_ARP
#define LWIP_ARP                        1
#endif

/* ARP_TABLE_SIZE: Number of active MAC-IP address pairs cached. */
#if !defined ARP_TABLE_SIZE
#define ARP_TABLE_SIZE                  10
#endif

/* The time an ARP entry stays valid after its last update,
 * for ARP_TMR_INTERVAL = 1000, this is
 * (60 * 5) seconds = 5 minutes. */
#if !defined ARP_MAXAGE
#define ARP_MAXAGE                      300
#endif

/* ARP_QUEUEING==1: Multiple outgoing packets are queued during hardware address
 * resolution. By default, only the most recent packet is queued per IP address.
 * This is sufficient for most protocols and mainly reduces TCP connection
 * startup time. Set this to 1 if you know your application sends more than one
 * packet in a row to an IP address that is not in the ARP cache. */
#if !defined ARP_QUEUEING
#define ARP_QUEUEING                    0
#endif

/* The maximum number of packets which may be queued for each
 * unresolved address by other network layers. Defaults to 3, 0 means disabled.
 * Old packets are dropped, new packets are queued. */
#if !defined ARP_QUEUE_LEN
#define ARP_QUEUE_LEN                   3
#endif

/* ETHARP_SUPPORT_VLAN==1: support receiving and sending ethernet packets with
 * VLAN header. See the description of LWIP_HOOK_VLAN_CHECK and
 * LWIP_HOOK_VLAN_SET hooks to check/set VLAN headers.
 * Additionally, you can define ETHARP_VLAN_CHECK to an u16_t VLAN ID to check.
 * If ETHARP_VLAN_CHECK is defined, only VLAN-traffic for this VLAN is accepted.
 * If ETHARP_VLAN_CHECK is not defined, all traffic is accepted.
 * Alternatively, define a function/define ETHARP_VLAN_CHECK_FN(eth_hdr, vlan)
 * that returns 1 to accept a packet or 0 to drop a packet. */
#if !defined ETHARP_SUPPORT_VLAN
#define ETHARP_SUPPORT_VLAN             0
#endif

/* LWIP_ETHERNET==1: enable ethernet support even though ARP might be disabled */
#if !defined LWIP_ETHERNET
#define LWIP_ETHERNET                   LWIP_ARP
#endif

/* ETH_PAD_SIZE: number of bytes added before the ethernet header to ensure
 * alignment of payload after that header. Since the header is 14 bytes long,
 * without this padding e.g. addresses in the IP header will not be aligned
 * on a 32-bit boundary, so setting this to 2 can speed up 32-bit-platforms. */
#if !defined ETH_PAD_SIZE
#define ETH_PAD_SIZE                    2
#endif

/* ETHARP_SUPPORT_STATIC_ENTRIES==1: enable code to support static ARP table
 * entries (using etharp_add_static_entry/etharp_remove_static_entry). */
#if !defined ETHARP_SUPPORT_STATIC_ENTRIES
#define ETHARP_SUPPORT_STATIC_ENTRIES   0
#endif

/* ETHARP_TABLE_MATCH_NETIF==1: Match netif for ARP table entries.
 * If disabled, duplicate IP address on multiple netifs are not supported
 * (but this should only occur for AutoIP). */
#if !defined ETHARP_TABLE_MATCH_NETIF
#define ETHARP_TABLE_MATCH_NETIF        !LWIP_SINGLE_NETIF
#endif

/* ---------- IP options ---------- */

/* LWIP_IPV4==1: Enable IPv4 */
#if !defined LWIP_IPV4
#define LWIP_IPV4                       1
#endif

/* IP_FORWARD==1: Enables the ability to forward IP packets across network
 * interfaces. If you are going to run lwIP on a device with only one network
 * interface, define this to 0. */
#if !defined IP_FORWARD
#define IP_FORWARD                      0
#endif

/* IP_REASSEMBLY==1: Reassemble incoming fragmented IP packets. Note that
 * this option does not affect outgoing packet sizes, which can be controlled
 * via IP_FRAG. */
#if !defined IP_REASSEMBLY
#define IP_REASSEMBLY                   1
#endif

/* IP_FRAG==1: Fragment outgoing IP packets if their size exceeds MTU. Note
 * that this option does not affect incoming packet sizes, which can be
 * controlled via IP_REASSEMBLY. */
#if !defined IP_FRAG
#define IP_FRAG                         1
#endif

/* IP_OPTIONS_ALLOWED: Defines the behavior for IP options.
 *      IP_OPTIONS_ALLOWED==0: All packets with IP options are dropped.
 *      IP_OPTIONS_ALLOWED==1: IP options are allowed (but not parsed). */
#if !defined IP_OPTIONS_ALLOWED
#define IP_OPTIONS_ALLOWED              1
#endif

/* IP_REASS_MAXAGE: Maximum time (in multiples of IP_TMR_INTERVAL - so seconds, normally)
 * a fragmented IP packet waits for all fragments to arrive. If not all fragments arrived
 * in this time, the whole packet is discarded. */
#if !defined IP_REASS_MAXAGE
#define IP_REASS_MAXAGE                 15
#endif

/* IP_REASS_MAX_PBUFS: Total maximum amount of pbufs waiting to be reassembled.
 * Since the received pbufs are enqueued, be sure to configure
 * PBUF_POOL_SIZE > IP_REASS_MAX_PBUFS so that the stack is still able to receive
 * packets even if the maximum amount of fragments is enqueued for reassembly!
 * When IPv4 *and* IPv6 are enabled, this even changes to
 * (PBUF_POOL_SIZE > 2 * IP_REASS_MAX_PBUFS)! */
#if !defined IP_REASS_MAX_PBUFS
#define IP_REASS_MAX_PBUFS              10
#endif

/* IP_DEFAULT_TTL: Default value for Time-To-Live used by transport layers. */
#if !defined IP_DEFAULT_TTL
#define IP_DEFAULT_TTL                  255
#endif

/* IP_SOF_BROADCAST=1: Use the SOF_BROADCAST field to enable broadcast
 * filter per pcb on udp and raw send operations. To enable broadcast filter
 * on recv operations, you also have to set IP_SOF_BROADCAST_RECV=1. */
#if !defined IP_SOF_BROADCAST
#define IP_SOF_BROADCAST                0
#endif

/* IP_SOF_BROADCAST_RECV (requires IP_SOF_BROADCAST=1) enable the broadcast
 * filter on recv operations. */
#if !defined IP_SOF_BROADCAST_RECV
#define IP_SOF_BROADCAST_RECV           0
#endif

/* IP_FORWARD_ALLOW_TX_ON_RX_NETIF==1: allow ip_forward() to send packets back
 * out on the netif where it was received. This should only be used for
 * wireless networks.
 * ATTENTION: When this is 1, make sure your netif driver correctly marks incoming
 * link-layer-broadcast/multicast packets as such using the corresponding pbuf flags! */
#if !defined IP_FORWARD_ALLOW_TX_ON_RX_NETIF
#define IP_FORWARD_ALLOW_TX_ON_RX_NETIF  0
#endif

/* ---------- ICMP options ---------- */

/* LWIP_ICMP==1: Enable ICMP module inside the IP stack.
 * Be careful, disable that make your product non-compliant to RFC1122. */
#if !defined LWIP_ICMP
#define LWIP_ICMP                       1
#endif

/* ICMP_TTL: Default value for Time-To-Live used by ICMP packets. */
#if !defined ICMP_TTL
#define ICMP_TTL                       (IP_DEFAULT_TTL)
#endif

/* LWIP_BROADCAST_PING==1: respond to broadcast pings (default is unicast only) */
#if !defined LWIP_BROADCAST_PING
#define LWIP_BROADCAST_PING             0
#endif

/* LWIP_MULTICAST_PING==1: respond to multicast pings (default is unicast only) */
#if !defined LWIP_MULTICAST_PING
#define LWIP_MULTICAST_PING             0
#endif

/* ---------- RAW options ---------- */

/* LWIP_RAW==1: Enable application layer to hook into the IP layer itself. */
#if !defined LWIP_RAW
#define LWIP_RAW                        1
#endif

/* LWIP_RAW: Default value for Time-To-Live used by raw packets. */
#if !defined RAW_TTL
#define RAW_TTL                         IP_DEFAULT_TTL
#endif

/* ---------- DHCP options ---------- */

/* LWIP_DHCP==1: Enable DHCP module. */
#if !defined LWIP_DHCP
#define LWIP_DHCP                       1
#endif

/* ---------- AUTOIP options ---------- */

/* LWIP_AUTOIP==1: Enable AUTOIP module. */
#if !defined LWIP_AUTOIP
#define LWIP_AUTOIP                     1
#endif

/* LWIP_DHCP_AUTOIP_COOP==1: Allow DHCP and AUTOIP to be both enabled on
 * the same interface at the same time. */
#if !defined LWIP_DHCP_AUTOIP_COOP
#define LWIP_DHCP_AUTOIP_COOP           1
#endif

/* LWIP_DHCP_AUTOIP_COOP_TRIES: Set to the number of DHCP DISCOVER probes
 * that should be sent before falling back on AUTOIP (the DHCP client keeps
 * running in this case). This can be set as low as 1 to get an AutoIP address
 * very  quickly, but you should be prepared to handle a changing IP address
 * when DHCP overrides AutoIP. */
#if !defined LWIP_DHCP_AUTOIP_COOP_TRIES
#define LWIP_DHCP_AUTOIP_COOP_TRIES     1
#endif

/* Defines the hardware random number generator for AUTOIP module. */
#if !defined LWIP_AUTOIP_RAND
#define LWIP_AUTOIP_RAND(netif)         ((u32_t) random_get())
#endif

/* Macro that generates the initial IP address to be tried by AUTOIP. */
#if !defined LWIP_AUTOIP_CREATE_SEED_ADDR
#define LWIP_AUTOIP_CREATE_SEED_ADDR(netif) \
  lwip_htonl(AUTOIP_RANGE_START + (random_get() % 0xE00))
#endif

/* ----- SNMP MIB2 support      ----- */

/* LWIP_MIB2_CALLBACKS==1: Turn on SNMP MIB2 callbacks.
 * Turn this on to get callbacks needed to implement MIB2.
 * Usually MIB2_STATS should be enabled, too. */
#if !defined LWIP_MIB2_CALLBACKS
#define LWIP_MIB2_CALLBACKS             0
#endif

/* ------- Multicast options ------- */

/* LWIP_MULTICAST_TX_OPTIONS==1: Enable multicast TX support like the socket options
 * IP_MULTICAST_TTL/IP_MULTICAST_IF/IP_MULTICAST_LOOP, as well as (currently only)
 * core support for the corresponding IPv6 options. */
#if !defined LWIP_MULTICAST_TX_OPTIONS
#define LWIP_MULTICAST_TX_OPTIONS       ((LWIP_IGMP || LWIP_IPV6_MLD) && (LWIP_UDP || LWIP_RAW))
#endif

/* --------- IGMP options --------- */

/* LWIP_IGMP==1: Turn on IGMP module. */
#if !defined LWIP_IGMP
#define LWIP_IGMP                       1
#endif

/* ---------- PTP options ---------- */

/* Enable support for timestamping packets to support PTP protocol. */
#if !defined LWIP_PTPD
#define LWIP_PTPD                       1
#endif

/* ---------- DNS options ----------- */

/* LWIP_DNS==1: Turn on DNS module. UDP must be available for DNS
 * transport. */
#if !defined LWIP_DNS
#define LWIP_DNS                        0
#endif

/* ---------- UDP options ---------- */

/* LWIP_UDP==1: Turn on UDP. */
#if !defined LWIP_UDP
#define LWIP_UDP                        1
#endif

/* LWIP_UDPLITE==1: Turn on UDP-Lite. (Requires LWIP_UDP) */
#if !defined LWIP_UDPLITE
#define LWIP_UDPLITE                    0
#endif

/* UDP_TTL: Default Time-To-Live value. */
#if !defined UDP_TTL
#define UDP_TTL                         IP_DEFAULT_TTL
#endif

/* LWIP_NETBUF_RECVINFO==1: append destination addr and port to every netbuf. */
#if !defined LWIP_NETBUF_RECVINFO
#define LWIP_NETBUF_RECVINFO            0
#endif

/* ---------- TCP options ---------- */

/* LWIP_TCP==1: Turn on TCP. */
#if !defined LWIP_TCP
#define LWIP_TCP                        1
#endif

/* TCP_TTL: Default Time-To-Live value. */
#if !defined TCP_TTL
#define TCP_TTL                         IP_DEFAULT_TTL
#endif

/* TCP receive window. */
#if !defined TCP_WND
#define TCP_WND                         (4 * TCP_MSS)
#endif

/* TCP_MAXRTX: Maximum number of retransmissions of data segments. */
#if !defined TCP_MAXRTX
#define TCP_MAXRTX                      12
#endif

/* TCP_SYNMAXRTX: Maximum number of retransmissions of SYN segments. */
#if !defined TCP_SYNMAXRTX
#define TCP_SYNMAXRTX                   6
#endif

/* Controls if TCP should queue segments that arrive out of
 * order. Define to 0 if your device is low on memory. */
#if !defined TCP_QUEUE_OOSEQ
#define TCP_QUEUE_OOSEQ                 0
#endif

/* LWIP_TCP_SACK_OUT==1: TCP will support sending selective acknowledgements (SACKs). */
#if !defined LWIP_TCP_SACK_OUT
#define LWIP_TCP_SACK_OUT               0
#endif

/* LWIP_TCP_MAX_SACK_NUM: The maximum number of SACK values to include in TCP segments.
 * Must be at least 1, but is only used if LWIP_TCP_SACK_OUT is enabled.
 * NOTE: Even though we never send more than 3 or 4 SACK ranges in a single segment
 * (depending on other options), setting this option to values greater than 4 is not pointless.
 * This is basically the max number of SACK ranges we want to keep track of.
 * As new data is delivered, some of the SACK ranges may be removed or merged.
 * In that case some of those older SACK ranges may be used again.
 * The amount of memory used to store SACK ranges is LWIP_TCP_MAX_SACK_NUM * 8 bytes for
 * each TCP PCB. */
#if !defined LWIP_TCP_MAX_SACK_NUM
#define LWIP_TCP_MAX_SACK_NUM           4
#endif

/* TCP_MSS: TCP Maximum segment size. (default is 536, a conservative
 * default, you might want to increase this.) For the receive side,
 * this MSS is advertised to the remote side when opening a connection.
 * For the transmit size, this MSS sets an upper limit on the MSS advertised
 * by the remote host. */
#if !defined TCP_MSS
#define TCP_MSS                         (1500 - 40)
#endif

/* TCP sender buffer space (bytes). */
#if !defined TCP_SND_BUF
#define TCP_SND_BUF                     (4 * TCP_MSS)
#endif

/* TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
 * as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work. */
#if !defined TCP_SND_QUEUELEN
#define TCP_SND_QUEUELEN                ((4 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
#endif

/* TCP_LISTEN_BACKLOG: Enable the backlog option for tcp listen pcb. */
#if !defined TCP_LISTEN_BACKLOG
#define TCP_LISTEN_BACKLOG              0
#endif

/* The maximum allowed backlog for TCP listen netconns.
 * This backlog is used unless another is explicitly specified.
 * 0xff is the maximum (u8_t). */
#if !defined TCP_DEFAULT_LISTEN_BACKLOG
#define TCP_DEFAULT_LISTEN_BACKLOG      0xff
#endif

/* TCP_OVERSIZE: The maximum number of bytes that tcp_write may
 * allocate ahead of time in an attempt to create shorter pbuf chains
 * for transmission. The meaningful range is 0 to TCP_MSS. Some
 * suggested values are:
 *
 * 0:         Disable oversized allocation. Each tcp_write() allocates a new
              pbuf (old behaviour).
 * 1:         Allocate size-aligned pbufs with minimal excess. Use this if your
 *            scatter-gather DMA requires aligned fragments.
 * 128:       Limit the pbuf/memory overhead to 20%.
 * TCP_MSS:   Try to create unfragmented TCP packets.
 * TCP_MSS/4: Try to create 4 fragments or less per TCP packet. */
#if !defined TCP_OVERSIZE
#define TCP_OVERSIZE                    TCP_MSS
#endif

/* LWIP_TCP_TIMESTAMPS==1: support the TCP timestamp option.
 * The timestamp option is currently only used to help remote hosts, it is not
 * really used locally. Therefore, it is only enabled when a TS option is
 * received in the initial SYN packet from a remote host. */
#if !defined LWIP_TCP_TIMESTAMPS
#define LWIP_TCP_TIMESTAMPS             0
#endif

/* LWIP_ALTCP==1: enable the altcp API.
 * altcp is an abstraction layer that prevents applications linking against the
 * tcp.h functions but provides the same functionality. It is used to e.g. add
 * SSL/TLS or proxy-connect support to an application written for the tcp callback
 * API without that application knowing the protocol details.
 *
 * With LWIP_ALTCP==0, applications written against the altcp API can still be
 * compiled but are directly linked against the tcp.h callback API and then
 * cannot use layered protocols. */
#if !defined LWIP_ALTCP
#define LWIP_ALTCP                      0
#endif

/* LWIP_ALTCP_TLS==1: enable TLS support for altcp API.
 * This needs a port of the functions in altcp_tls.h to a TLS library.
 * A port to ARM mbedtls is provided with lwIP, see apps/altcp_tls/ directory
 * and LWIP_ALTCP_TLS_MBEDTLS option. */
#if !defined LWIP_ALTCP_TLS
#define LWIP_ALTCP_TLS                  0
#endif

/* ---------- Pbuf options ---------- */


/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. The default is
 * designed to accomodate single full size TCP frame in one pbuf, including
 * TCP_MSS, IP header, and link header. */
#if !defined PBUF_POOL_BUFSIZE
#define PBUF_POOL_BUFSIZE       LWIP_MEM_ALIGN_SIZE(TCP_MSS + 40 + PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN)
#endif

/* ---------- Network Interfaces options ---------- */

/* LWIP_SINGLE_NETIF==1: use a single netif only. This is the common case for
 * small real-life targets. Some code like routing etc. can be left out. */
#if !defined LWIP_SINGLE_NETIF
#define LWIP_SINGLE_NETIF               1
#endif

/* LWIP_NETIF_HOSTNAME==1: use DHCP_OPTION_HOSTNAME with netif's hostname
 * field. */
#if !defined LWIP_NETIF_HOSTNAME
#define LWIP_NETIF_HOSTNAME             0
#endif

/* LWIP_NETIF_API==1: Support netif api (in netifapi.c) */
#if !defined LWIP_NETIF_API
#define LWIP_NETIF_API                  1
#endif

/* LWIP_NETIF_STATUS_CALLBACK==1: Support a callback function whenever an interface
 * changes its up/down status (i.e., due to DHCP IP acquisition) */
#if !defined LWIP_NETIF_STATUS_CALLBACK
#define LWIP_NETIF_STATUS_CALLBACK      1
#endif

/* LWIP_NETIF_EXT_STATUS_CALLBACK==1: Support an extended callback function 
 * for several netif related event that supports multiple subscribers. */
#if !defined LWIP_NETIF_EXT_STATUS_CALLBACK
#define LWIP_NETIF_EXT_STATUS_CALLBACK  0
#endif

/* LWIP_NETIF_LINK_CALLBACK==1: Support a callback function from an interface
 * whenever the link changes (i.e., link down) */
#if !defined LWIP_NETIF_LINK_CALLBACK
#define LWIP_NETIF_LINK_CALLBACK        1
#endif

/* LWIP_NETIF_REMOVE_CALLBACK==1: Support a callback function that is called
 * when a netif has been removed */
#if !defined LWIP_NETIF_REMOVE_CALLBACK
#define LWIP_NETIF_REMOVE_CALLBACK      0
#endif

/* LWIP_NETIF_HWADDRHINT==1: Cache link-layer-address hints (e.g. table
 * indices) in struct netif. TCP and UDP can make use of this to prevent
 * scanning the ARP table for every sent packet. While this is faster for big
 * ARP tables or many concurrent connections, it might be counterproductive
 * if you have a tiny ARP table or if there never are concurrent connections. */
#if !defined LWIP_NETIF_HWADDRHINT
#define LWIP_NETIF_HWADDRHINT           0
#endif

/* LWIP_NUM_NETIF_CLIENT_DATA: Number of clients that may store
 * data in client_data member array of struct netif. At least one
 * is needed for MDNS responder services. */
#if !defined LWIP_NUM_NETIF_CLIENT_DATA
#define LWIP_NUM_NETIF_CLIENT_DATA      1
#endif

/* ---------- LOOPIF options ---------- */

/* LWIP_HAVE_LOOPIF==1: Support loop interface (127.0.0.1).
 * This is only needed when no real netifs are available. If at least one other
 * netif is available, loopback traffic uses this netif. */
#if !defined LWIP_HAVE_LOOPIF
#define LWIP_HAVE_LOOPIF                (LWIP_NETIF_LOOPBACK && !LWIP_SINGLE_NETIF)
#endif

/* LWIP_LOOPIF_MULTICAST==1: Support multicast/IGMP on loop interface (127.0.0.1). */
#if !defined LWIP_LOOPIF_MULTICAST
#define LWIP_LOOPIF_MULTICAST           0
#endif

/* LWIP_NETIF_LOOPBACK==1: Support sending packets with a destination IP
 * address equal to the netif IP address, looping them back up the stack. */
#if !defined LWIP_NETIF_LOOPBACK
#define LWIP_NETIF_LOOPBACK             0
#endif

/* ---------- Thread options ---------- */

/* TCPIP_THREAD_NAME: The name assigned to the main tcpip thread. */
#if !defined TCPIP_THREAD_NAME
#define TCPIP_THREAD_NAME              "TCPIP"
#endif

/* TCPIP_THREAD_STACKSIZE: The stack size used by the main tcpip thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created. */
#if !defined TCPIP_THREAD_STACKSIZE
#define TCPIP_THREAD_STACKSIZE          (4 * 1024)
#endif

/* TCPIP_THREAD_PRIO: The priority assigned to the main tcpip thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created. */
#if !defined TCPIP_THREAD_PRIO
#define TCPIP_THREAD_PRIO               (osPriorityHigh)
#endif

/* TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called. */
#if !defined TCPIP_MBOX_SIZE
#define TCPIP_MBOX_SIZE                 32
#endif

/* DEFAULT_THREAD_NAME: The name assigned to any other lwIP thread. */
#if !defined DEFAULT_THREAD_NAME
#define DEFAULT_THREAD_NAME             "LWIP"
#endif

/* DEFAULT_THREAD_STACKSIZE: The stack size used by any other lwIP thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created. */
#if !defined DEFAULT_THREAD_STACKSIZE
#define DEFAULT_THREAD_STACKSIZE        (2 * 1024)
#endif

/* DEFAULT_THREAD_PRIO: The priority assigned to any other lwIP thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created. */
#if !defined DEFAULT_THREAD_PRIO
#define DEFAULT_THREAD_PRIO             (osPriorityNormal)
#endif

/* DEFAULT_RAW_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_RAW. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created. */
#if !defined DEFAULT_RAW_RECVMBOX_SIZE
#define DEFAULT_RAW_RECVMBOX_SIZE       12
#endif

/* DEFAULT_UDP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_UDP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created. */
#if !defined DEFAULT_UDP_RECVMBOX_SIZE
#define DEFAULT_UDP_RECVMBOX_SIZE       12
#endif

/* DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created. */
#if !defined DEFAULT_TCP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE       12
#endif

/* DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when the acceptmbox is created. */
#if !defined DEFAULT_ACCEPTMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE         12
#endif

/* TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called. */
#if !defined TCPIP_MBOX_SIZE
#define TCPIP_MBOX_SIZE                 32
#endif

/* Define this to something that triggers a watchdog. This is called from
 * tcpip_thread after processing a message. */
#if !defined LWIP_TCPIP_THREAD_ALIVE
#define LWIP_TCPIP_THREAD_ALIVE()
#endif

/* ---------- Sequential layer options ---------- */

/* Enable Netconn API (require to use api_lib.c). */
#if !defined LWIP_NETCONN
#define LWIP_NETCONN                    1
#endif

/* Enable tcpip_timeout/tcpip_untimeout to create timers running in 
 * tcpip_thread from another thread. */
#if !defined LWIP_TCPIP_TIMEOUT
#define LWIP_TCPIP_TIMEOUT              0
#endif

/* ---------- Socket options ---------- */

/* LWIP_SOCKET==1: Enable Socket API (require to use sockets.c) */
#if !defined LWIP_SOCKET
#define LWIP_SOCKET                     0
#endif

/* LWIP_SOCKET_SET_ERRNO==1: Set errno when socket functions cannot complete
 * successfully, as required by POSIX. Default is POSIX-compliant. */
#if !defined LWIP_SOCKET_SET_ERRNO
#define LWIP_SOCKET_SET_ERRNO           1
#endif

/* LWIP_COMPAT_SOCKETS==1: Enable BSD-style sockets functions names through defines.
 * LWIP_COMPAT_SOCKETS==2: Same as ==1 but correctly named functions are created.
 * While this helps code completion, it might conflict with existing libraries.
 * (only used if you use sockets.c) */
#if !defined LWIP_COMPAT_SOCKETS
#define LWIP_COMPAT_SOCKETS             1
#endif

/* LWIP_POSIX_SOCKETS_IO_NAMES==1: Enable POSIX-style sockets functions names.
 * Disable this option if you use a POSIX operating system that uses the same
 * names (read, write & close). (only used if you use sockets.c) */
#if !defined LWIP_POSIX_SOCKETS_IO_NAMES
#define LWIP_POSIX_SOCKETS_IO_NAMES     1
#endif

/* LWIP_SOCKET_OFFSET==n: Increases the file descriptor number created by LwIP with n.
 * This can be useful when there are multiple APIs which create file descriptors.
 * When they all start with a different offset and you won't make them overlap you can
 * re implement read/write/close/ioctl/fnctl to send the requested action to the right
 * library (sharing select will need more work though). */
#if !defined LWIP_SOCKET_OFFSET
#define LWIP_SOCKET_OFFSET              0
#endif

/* LWIP_TCP_KEEPALIVE==1: Enable TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPCNT
 * options processing. Note that TCP_KEEPIDLE and TCP_KEEPINTVL have to be set
 * in seconds. (does not require sockets.c, and will affect tcp.c) */
#if !defined LWIP_TCP_KEEPALIVE
#define LWIP_TCP_KEEPALIVE              1
#endif

/* LWIP_SO_SNDTIMEO==1: Enable send timeout for sockets/netconns and
 * SO_SNDTIMEO processing. */
#if !defined LWIP_SO_SNDTIMEO
#define LWIP_SO_SNDTIMEO                0
#endif

/* LWIP_SO_RCVTIMEO==1: Enable receive timeout for sockets/netconns and
 * SO_RCVTIMEO processing. */
#if !defined LWIP_SO_RCVTIMEO
#define LWIP_SO_RCVTIMEO                0
#endif

/* LWIP_SO_SNDRCVTIMEO_NONSTANDARD==1: SO_RCVTIMEO/SO_SNDTIMEO take an int
 * (milliseconds, much like winsock does) instead of a struct timeval (default). */
#if !defined LWIP_SO_SNDRCVTIMEO_NONSTANDARD
#define LWIP_SO_SNDRCVTIMEO_NONSTANDARD 0
#endif

/* LWIP_SO_RCVBUF==1: Enable SO_RCVBUF processing. */
#if !defined LWIP_SO_RCVBUF
#define LWIP_SO_RCVBUF                  0
#endif

/* LWIP_SO_LINGER==1: Enable SO_LINGER processing. */
#if !defined LWIP_SO_LINGER
#define LWIP_SO_LINGER                  0
#endif

/* If LWIP_SO_RCVBUF is used, this is the default value for recv_bufsize. */
#if !defined RECV_BUFSIZE_DEFAULT
#define RECV_BUFSIZE_DEFAULT            INT_MAX
#endif

/* By default, TCP socket/netconn close waits 20 seconds max to send the FIN. */
#if !defined LWIP_TCP_CLOSE_TIMEOUT_MS_DEFAULT
#define LWIP_TCP_CLOSE_TIMEOUT_MS_DEFAULT 20000
#endif

/* SO_REUSE==1: Enable SO_REUSEADDR option. */
#if !defined SO_REUSE
#define SO_REUSE                        0
#endif

/* ---------- Statistics options ---------- */

/* LWIP_STATS==1: Enable statistics collection in lwip_stats. */
#if !defined LWIP_STATS
#define LWIP_STATS                      0
#endif

#if LWIP_STATS

/* LWIP_STATS_DISPLAY==1: Compile in the statistics output functions. */
#if !defined LWIP_STATS_DISPLAY
#define LWIP_STATS_DISPLAY              1
#endif

/* LINK_STATS==1: Enable link stats. */
#if !defined LINK_STATS
#define LINK_STATS                      1
#endif

/* ETHARP_STATS==1: Enable etharp stats. */
#if !defined ETHARP_STATS
#define ETHARP_STATS                    (LWIP_ARP)
#endif

/* IP_STATS==1: Enable IP stats. */
#if !defined IP_STATS
#define IP_STATS                        1
#endif

/* IPFRAG_STATS==1: Enable IP fragmentation stats. Default is
 * on if using either frag or reass. */
#if !defined IPFRAG_STATS
#define IPFRAG_STATS                    (IP_REASSEMBLY || IP_FRAG)
#endif

/* ICMP_STATS==1: Enable ICMP stats. */
#if !defined ICMP_STATS
#define ICMP_STATS                      1
#endif

/* IGMP_STATS==1: Enable IGMP stats. */
#if !defined IGMP_STATS
#define IGMP_STATS                      (LWIP_IGMP)
#endif

/* UDP_STATS==1: Enable UDP stats. Default is on if
 * UDP enabled, otherwise off. */
#if !defined UDP_STATS
#define UDP_STATS                       (LWIP_UDP)
#endif

/* TCP_STATS==1: Enable TCP stats. Default is on if TCP
 * enabled, otherwise off. */
#if !defined TCP_STATS
#define TCP_STATS                       (LWIP_TCP)
#endif

/* MEM_STATS==1: Enable mem.c stats. */
#if !defined MEM_STATS
#define MEM_STATS                       ((MEM_LIBC_MALLOC == 0) && (MEM_USE_POOLS == 0))
#endif

/* MEMP_STATS==1: Enable memp.c pool stats. */
#if !defined MEMP_STATS
#define MEMP_STATS                      (MEMP_MEM_MALLOC == 0)
#endif

/* SYS_STATS==1: Enable system stats (sem and mbox counts, etc). */
#if !defined SYS_STATS
#define SYS_STATS                       (NO_SYS == 0)
#endif

/* IP6_STATS==1: Enable IPv6 stats. */
#if !defined IP6_STATS
#define IP6_STATS                       (LWIP_IPV6)
#endif

/* ICMP6_STATS==1: Enable ICMP for IPv6 stats. */
#if !defined ICMP6_STATS
#define ICMP6_STATS                     (LWIP_IPV6 && LWIP_ICMP6)
#endif

/* IP6_FRAG_STATS==1: Enable IPv6 fragmentation stats. */
#if !defined IP6_FRAG_STATS
#define IP6_FRAG_STATS                  (LWIP_IPV6 && (LWIP_IPV6_FRAG || LWIP_IPV6_REASS))
#endif

/* MLD6_STATS==1: Enable MLD for IPv6 stats. */
#if !defined MLD6_STATS
#define MLD6_STATS                      (LWIP_IPV6 && LWIP_IPV6_MLD)
#endif

/* ND6_STATS==1: Enable Neighbor discovery for IPv6 stats. */
#if !defined ND6_STATS
#define ND6_STATS                       (LWIP_IPV6)
#endif

/* MIB2_STATS==1: Stats for SNMP MIB2. */
#if !defined MIB2_STATS
#define MIB2_STATS                      0
#endif

#endif

/* ---------- Checksum options ---------- */

/* The STM32F4x7 allows computing and verifying the IP, UDP, TCP and ICMP checksums by hardware:
 * - To use this feature let the following define uncommented.
 * - To disable it and process by CPU comment the  the checksum. */
#if !defined CHECKSUM_BY_HARDWARE
#define CHECKSUM_BY_HARDWARE 
#endif

#ifdef CHECKSUM_BY_HARDWARE

/* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets. */
#if !defined CHECKSUM_GEN_IP
#define CHECKSUM_GEN_IP                 0
#endif

/* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets. */
#if !defined CHECKSUM_GEN_UDP
#define CHECKSUM_GEN_UDP                0
#endif

/* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets. */
#if !defined CHECKSUM_GEN_TCP
#define CHECKSUM_GEN_TCP                0
#endif

/* CHECKSUM_GEN_ICMP==1: Generate checksums in software for outgoing ICMP packets. */
#if !defined CHECKSUM_GEN_ICMP
#define CHECKSUM_GEN_ICMP               0
#endif

/* CHECKSUM_GEN_ICMP6==1: Generate checksums in software for outgoing ICMP6 packets. */
#if !defined CHECKSUM_GEN_ICMP6
#define CHECKSUM_GEN_ICMP6              0
#endif

/* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets. */
#if !defined CHECKSUM_CHECK_IP
#define CHECKSUM_CHECK_IP               0
#endif

/* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets. */
#if !defined CHECKSUM_CHECK_UDP
#define CHECKSUM_CHECK_UDP              0
#endif

/* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets. */
#if !defined CHECKSUM_CHECK_TCP
#define CHECKSUM_CHECK_TCP              0
#endif

/* CHECKSUM_CHECK_ICMP==1: Check checksums in software for incoming ICMP packets. */
#if !defined CHECKSUM_CHECK_ICMP
#define CHECKSUM_CHECK_ICMP             0
#endif

/* CHECKSUM_CHECK_ICMP6==1: Check checksums in software for incoming ICMPv6 packets */
#if !defined CHECKSUM_CHECK_ICMP6
#define CHECKSUM_CHECK_ICMP6            0
#endif

#else

/* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets. */
#if !defined CHECKSUM_GEN_IP
#define CHECKSUM_GEN_IP                 1
#endif

/* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets. */
#if !defined CHECKSUM_GEN_UDP
#define CHECKSUM_GEN_UDP                1
#endif

/* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets. */
#if !defined CHECKSUM_GEN_TCP
#define CHECKSUM_GEN_TCP                1
#endif

/* CHECKSUM_GEN_ICMP==1: Generate checksums in software for outgoing ICMP packets. */
#if !defined CHECKSUM_GEN_ICMP
#define CHECKSUM_GEN_ICMP               1
#endif

/* CHECKSUM_GEN_ICMP6==1: Generate checksums in software for outgoing ICMP6 packets. */
#if !defined CHECKSUM_GEN_ICMP6
#define CHECKSUM_GEN_ICMP6              1
#endif

/* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets. */
#if !defined CHECKSUM_CHECK_IP
#define CHECKSUM_CHECK_IP               1
#endif

/* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets. */
#if !defined CHECKSUM_CHECK_UDP
#define CHECKSUM_CHECK_UDP              1
#endif

/* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets. */
#if !defined CHECKSUM_CHECK_TCP
#define CHECKSUM_CHECK_TCP              1
#endif

/* CHECKSUM_CHECK_ICMP==1: Check checksums in software for incoming ICMP packets. */
#if !defined CHECKSUM_CHECK_ICMP
#define CHECKSUM_CHECK_ICMP             1
#endif

/* CHECKSUM_CHECK_ICMP6==1: Check checksums in software for incoming ICMPv6 packets */
#if !defined CHECKSUM_CHECK_ICMP6
#define CHECKSUM_CHECK_ICMP6            1
#endif

#endif

/* LWIP_CHECKSUM_ON_COPY==1: Calculate checksum when copying data from
 * application buffers to pbufs. */
#if !defined LWIP_CHECKSUM_ON_COPY
#define LWIP_CHECKSUM_ON_COPY           0
#endif

/* ---------- IPv6 options --------------- */

/* LWIP_IPV6==1: Enable IPv6. */
#if !defined LWIP_IPV6
#define LWIP_IPV6                       0
#endif

/* ---------- Application options ---------- */

/* LWIP_MDNS_RESPONDER==1: Turn on multicast DNS module. UDP must be
 * available for MDNS transport. IGMP is needed for IPv4 multicast. */
#if !defined LWIP_MDNS_RESPONDER
#define LWIP_MDNS_RESPONDER             1
#endif

/* ---------- Debugging options ---------- */

#if 0
#define LWIP_DEBUG
#endif

#ifdef LWIP_DEBUG
#define U8_F "c"
#define S8_F "c"
#define X8_F "02x"
#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "u"
#endif

/* LWIP_DBG_MIN_LEVEL: After masking, the value of the debug is
 * compared against this value. If it is smaller, then debugging
 * messages are written. */
#if !defined LWIP_DBG_MIN_LEVEL
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL
#endif

/* LWIP_DBG_TYPES_ON: A mask that can be used to globally enable/disable
 * debug messages of certain types. */
#if !defined LWIP_DBG_TYPES_ON
#define LWIP_DBG_TYPES_ON               LWIP_DBG_OFF
#endif

/* ETHARP_DEBUG: Enable debugging in etharp.c. */
#if !defined ETHARP_DEBUG
#define ETHARP_DEBUG                    LWIP_DBG_OFF
#endif

/* NETIF_DEBUG: Enable debugging in netif.c. */
#if !defined NETIF_DEBUG
#define NETIF_DEBUG                     LWIP_DBG_OFF
#endif

/* PBUF_DEBUG: Enable debugging in pbuf.c. */
#if !defined PBUF_DEBUG
#define PBUF_DEBUG                      LWIP_DBG_OFF
#endif

/* API_LIB_DEBUG: Enable debugging in api_lib.c. */
#if !defined API_LIB_DEBUG
#define API_LIB_DEBUG                   LWIP_DBG_OFF
#endif

/* API_MSG_DEBUG: Enable debugging in api_msg.c. */
#if !defined API_MSG_DEBUG
#define API_MSG_DEBUG                   LWIP_DBG_OFF
#endif

/* SOCKETS_DEBUG: Enable debugging in sockets.c. */
#if !defined SOCKETS_DEBUG
#define SOCKETS_DEBUG                   LWIP_DBG_OFF
#endif

/* ICMP_DEBUG: Enable debugging in icmp.c. */
#if !defined ICMP_DEBUG
#define ICMP_DEBUG                      LWIP_DBG_OFF
#endif

/* IGMP_DEBUG: Enable debugging in igmp.c. */
#if !defined IGMP_DEBUG
#define IGMP_DEBUG                      LWIP_DBG_OFF
#endif

/* INET_DEBUG: Enable debugging in inet.c. */
#if !defined INET_DEBUG
#define INET_DEBUG                      LWIP_DBG_OFF
#endif

/* IP_DEBUG: Enable debugging for IP. */
#if !defined IP_DEBUG
#define IP_DEBUG                        LWIP_DBG_OFF
#endif

/* IP_REASS_DEBUG: Enable debugging in ip_frag.c for both frag & reass. */
#if !defined IP_REASS_DEBUG
#define IP_REASS_DEBUG                  LWIP_DBG_OFF
#endif

/* RAW_DEBUG: Enable debugging in raw.c. */
#if !defined RAW_DEBUG
#define RAW_DEBUG                       LWIP_DBG_OFF
#endif

/* MEM_DEBUG: Enable debugging in mem.c. */
#if !defined MEM_DEBUG
#define MEM_DEBUG                       LWIP_DBG_OFF
#endif

/* MEMP_DEBUG: Enable debugging in memp.c. */
#if !defined MEMP_DEBUG
#define MEMP_DEBUG                      LWIP_DBG_OFF
#endif

/* SYS_DEBUG: Enable debugging in sys.c. */
#if !defined SYS_DEBUG
#define SYS_DEBUG                       LWIP_DBG_OFF
#endif

/* TIMERS_DEBUG: Enable debugging in timers.c. */
#if !defined TIMERS_DEBUG
#define TIMERS_DEBUG                    LWIP_DBG_OFF
#endif

/* TCP_DEBUG: Enable debugging for TCP. */
#if !defined TCP_DEBUG
#define TCP_DEBUG                       LWIP_DBG_OFF
#endif

/* TCP_INPUT_DEBUG: Enable debugging in tcp_in.c for incoming debug. */
#if !defined TCP_INPUT_DEBUG
#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF
#endif

/* TCP_FR_DEBUG: Enable debugging in tcp_in.c for fast retransmit. */
#if !defined TCP_FR_DEBUG
#define TCP_FR_DEBUG                    LWIP_DBG_OFF
#endif

/* TCP_RTO_DEBUG: Enable debugging in TCP for retransmit timeout. */
#if !defined TCP_RTO_DEBUG
#define TCP_RTO_DEBUG                   LWIP_DBG_OFF
#endif

/* TCP_CWND_DEBUG: Enable debugging for TCP congestion window. */
#if !defined TCP_CWND_DEBUG
#define TCP_CWND_DEBUG                  LWIP_DBG_OFF
#endif

/* TCP_WND_DEBUG: Enable debugging in tcp_in.c for window updating. */
#if !defined TCP_WND_DEBUG
#define TCP_WND_DEBUG                   LWIP_DBG_OFF
#endif

/* TCP_OUTPUT_DEBUG: Enable debugging in tcp_out.c output functions. */
#if !defined TCP_OUTPUT_DEBUG
#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF
#endif

/* TCP_RST_DEBUG: Enable debugging for TCP with the RST message. */
#if !defined TCP_RST_DEBUG
#define TCP_RST_DEBUG                   LWIP_DBG_OFF
#endif

/* TCP_QLEN_DEBUG: Enable debugging for TCP queue lengths. */
#if !defined TCP_QLEN_DEBUG
#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF
#endif

/* UDP_DEBUG: Enable debugging in UDP. */
#if !defined UDP_DEBUG
#define UDP_DEBUG                       LWIP_DBG_OFF
#endif

/* TCPIP_DEBUG: Enable debugging in tcpip.c. */
#if !defined TCPIP_DEBUG
#define TCPIP_DEBUG                     LWIP_DBG_OFF
#endif

/* SLIP_DEBUG: Enable debugging in slipif.c. */
#if !defined SLIP_DEBUG
#define SLIP_DEBUG                      LWIP_DBG_OFF
#endif

/* DHCP_DEBUG: Enable debugging in dhcp.c. */
#if !defined DHCP_DEBUG
#define DHCP_DEBUG                      LWIP_DBG_OFF
#endif

/* AUTOIP_DEBUG: Enable debugging in autoip.c. */
#if !defined AUTOIP_DEBUG
#define AUTOIP_DEBUG                    LWIP_DBG_OFF
#endif

/* DNS_DEBUG: Enable debugging for DNS. */
#if !defined DNS_DEBUG
#define DNS_DEBUG                       LWIP_DBG_OFF
#endif

/* IP6_DEBUG: Enable debugging for IPv6. */
#if !defined IP6_DEBUG
#define IP6_DEBUG                       LWIP_DBG_OFF
#endif

/* MDNS_DEBUG: Enable debugging for MDNS. */
#if !defined MDNS_DEBUG
#define MDNS_DEBUG                      LWIP_DBG_OFF
#endif

/* ---------- Performance tracking options ---------- */

/* LWIP_PERF: Enable performance testing for lwIP
 * (if enabled, arch/perf.h is included) */
#if !defined LWIP_PERF
#define LWIP_PERF                       0
#endif

#endif /* __LWIPOPTS_SHARED_H__ */
