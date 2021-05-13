// Embedded implementation of PTPd v2.
// Full implementation of IEEE 1588 - 2008 standard of ordinary clock.
// Derived from version PTPd 2.0.1 (17 Nov. 2010).
// Created by Van Kempen Alexandre.

#ifndef __PTPD_H__
#define __PTPD_H__

#if defined(USE_HAL_DRIVER)
#include "hal_system.h"
#endif
#include "cmsis_compiler.h"
#include "lwip/opt.h"
#include "network.h"
#include "outputf.h"

#if LWIP_PTPD

/* #define PTPD_DBGVV */
/* #define PTPD_DBGV */
/* #define PTPD_DBG */
/* #define PTPD_ERR */

#if 0
#define PTPD_DBGVV
#define PTPD_DBGV
#define PTPD_DBG
#define PTPD_ERR
#endif

#include "ptpd_constants.h"
#include "ptpd_datatypes.h"

// Debug messages.
#ifdef PTPD_DBGVV
#define PTPD_DBGV
#define PTPD_DBG
#define PTPD_ERR
#define DBGVV(...) __printf("(V) " __VA_ARGS__)
#else
#define DBGVV(...)
#endif

#ifdef PTPD_DBGV
#define PTPD_DBG
#define PTPD_ERR
#define DBGV(...)  { TimeInternal tmpTime; ptpd_get_time(&tmpTime); __printf("(d %d.%09d) ", tmpTime.seconds, tmpTime.nanoseconds); __printf(__VA_ARGS__); }
#else
#define DBGV(...)
#endif

#ifdef PTPD_DBG
#define PTPD_ERR
#define DBG(...)  { TimeInternal tmpTime; ptpd_get_time(&tmpTime); __printf("(D %d.%09d) ", tmpTime.seconds, tmpTime.nanoseconds); __printf(__VA_ARGS__); }
#else
#define DBG(...)
#endif

//
// System messages.
//
#ifdef PTPD_ERR
#define ERROR(...)  { TimeInternal tmpTime; ptpd_get_time(&tmpTime); __printf("(E %d.%09d) ", tmpTime.seconds, tmpTime.nanoseconds); __printf(__VA_ARGS__); }
/* #define ERROR(...)  { __printf("(E) "); __printf(__VA_ARGS__); } */
#else
#define ERROR(...)
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#define PTPD_LSBF
#elif BYTE_ORDER == BIG_ENDIAN
#define PTPD_MSBF
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Application management.
void ptpd_alert(void);
void ptpd_init(bool slave_only);
uint32_t ptpd_get_state(void);

// Protocol engine.
void ptpd_protocol_do_state(PtpClock*);
void ptpd_protocol_to_state(PtpClock*, uint8_t);

//  Best Master Clock (BMC) algorithm functions.
uint8_t ptpd_bmc(PtpClock*);
void ptpd_m1(PtpClock*);
void ptpd_p1(PtpClock*);
void ptpd_s1(PtpClock*, const MsgHeader*, const MsgAnnounce*);
void ptpd_clock_init(PtpClock*);
bool ptpd_is_same_port_identity(const PortIdentity*, const PortIdentity*);
void ptpd_add_foreign(PtpClock*, const MsgHeader*, const MsgAnnounce*);

// Message packing and unpacking functions.
void ptpd_msg_unpack_header(const octet_t*, MsgHeader*);
void ptpd_msg_unpack_announce(const octet_t*, MsgAnnounce*);
void ptpd_msg_unpack_sync(const octet_t*, MsgSync*);
void ptpd_msg_unpack_follow_up(const octet_t*, MsgFollowUp*);
void ptpd_msg_unpack_delay_req(const octet_t*, MsgDelayReq*);
void ptpd_msg_unpack_delay_resp(const octet_t*, MsgDelayResp*);
void ptpd_msg_unpack_peer_delay_req(const octet_t*, MsgPDelayReq*);
void ptpd_msg_unpack_peer_delay_resp(const octet_t*, MsgPDelayResp*);
void ptpd_msg_unpack_peer_delay_resp_follow_up(const octet_t*, MsgPDelayRespFollowUp*);
void ptpd_msg_pack_header(const PtpClock*, octet_t*);
void ptpd_msg_pack_announce(const PtpClock*, octet_t*);
void ptpd_msg_pack_sync(const PtpClock*, octet_t*, const Timestamp*);
void ptpd_msg_pack_follow_up(const PtpClock*, octet_t*, const Timestamp*);
void ptpd_msg_pack_delay_req(const PtpClock*, octet_t*, const Timestamp*);
void ptpd_msg_pack_delay_resp(const PtpClock*, octet_t*, const MsgHeader*, const Timestamp*);
void ptpd_msg_pack_peer_delay_req(const PtpClock*, octet_t*, const Timestamp*);
void ptpd_msg_pack_peer_delay_resp(octet_t*, const MsgHeader*, const Timestamp*);
void ptpd_msg_pack_peer_delay_resp_follow_up(octet_t*, const MsgHeader*, const Timestamp*);

// Network functions.
bool  ptpd_net_init(NetPath*, PtpClock*);
bool  ptpd_net_shutdown(NetPath*);
int32_t ptpd_net_select(NetPath*, const TimeInternal*);
ssize_t ptpd_net_recv_event(NetPath*, octet_t*, TimeInternal*);
ssize_t ptpd_net_recv_general(NetPath*, octet_t*, TimeInternal*);
ssize_t ptpd_net_send_event(NetPath*, const octet_t*, int16_t, TimeInternal*);
ssize_t ptpd_net_send_general(NetPath*, const octet_t*, int16_t);
ssize_t ptpd_net_send_peer_general(NetPath*, const octet_t*, int16_t);
ssize_t ptpd_net_send_peer_event(NetPath*, const octet_t*, int16_t, TimeInternal*);
void ptpd_net_empty_event_queue(NetPath *netPath);

// Precions time adjustment functions.
void ptpd_servo_init_clock(PtpClock*);
void ptpd_servo_update_peer_delay(PtpClock*, const TimeInternal*, bool);
void ptpd_servo_update_delay(PtpClock*, const TimeInternal*, const TimeInternal*, const TimeInternal*);
void ptpd_servo_update_offset(PtpClock *, const TimeInternal*, const TimeInternal*, const TimeInternal*);
void ptpd_servo_update_clock(PtpClock*);

// System precision time functions.
uint32_t ptpd_get_rand(uint32_t);
void ptpd_get_time(TimeInternal*);
void ptpd_set_time(const TimeInternal*);
bool ptpd_adj_freq(int32_t);

// Timer management functions.
void ptpd_timer_init(void);
void ptpd_timer_start(int32_t, uint32_t);
void ptpd_timer_stop(int32_t);
bool ptpd_timer_expired(int32_t);

// Timing management and arithmetic functions.
void ptpd_scaled_nanoseconds_to_internal_time(TimeInternal*, const int64_t*);
void ptpd_from_internal_time(const TimeInternal*, Timestamp*);
void ptpd_to_internal_time(TimeInternal*, const Timestamp*);
void ptpd_add_time(TimeInternal*, const TimeInternal*, const TimeInternal*);
void ptpd_sub_time(TimeInternal*, const TimeInternal*, const TimeInternal*);
void ptpd_div2_time(TimeInternal*);
int32_t ptpd_floor_log2(uint32_t);

// Return maximum of two numbers.
__STATIC_INLINE int32_t max(int32_t a, int32_t b)
{
  return a > b ? a : b;
}

// Return minimum of two numbers.
__STATIC_INLINE int32_t min(int32_t a, int32_t b)
{
  return a > b ? b : a;
}

// Endian corrections.
#if defined(PTPD_MSBF)
#define shift8(x,y)                     ((x) << ((3 - y) << 3))
#define shift16(x,y)                    ((x) << ((1 - y) << 4))
#elif defined(PTPD_LSBF)
#define shift8(x,y)                     ((x) << ((y) << 3))
#define shift16(x,y)                    ((x) << ((y) << 4))
#endif
#define flip16(x)                       htons(x)
#define flip32(x)                       htonl(x)

// Bit array manipulations.
#define get_flag(flag_field, mask)      ((bool) (((flag_field) & (mask)) == (mask)))
#define set_flag(flag_field, mask)      (flag_field) |= (mask)
#define clear_flag(flag_field, mask)    (flag_field) &= ~(mask)

#ifdef __cplusplus
}
#endif

#endif // LWIP_PTPD

#endif // __PTPD_H__
