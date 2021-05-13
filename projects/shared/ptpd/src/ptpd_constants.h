#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include "cmsis_os2.h"
#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default values and constants.

// 5.3.4 ClockIdentity
#define CLOCK_IDENTITY_LENGTH           8

// Implementation defaults.
#define DEFAULT_INBOUND_LATENCY         0       // In nanoseconds.
#define DEFAULT_OUTBOUND_LATENCY        0       // In nanoseconds.
#define DEFAULT_NO_RESET_CLOCK          false
#define DEFAULT_DOMAIN_NUMBER           0
#define DEFAULT_DELAY_MECHANISM         E2E
#define DEFAULT_AP                      2
#define DEFAULT_AI                      16
#define DEFAULT_DELAY_S                 6       // Exponencial smoothing - 2^s
#define DEFAULT_OFFSET_S                1       // Exponencial smoothing - 2^s
#define DEFAULT_ANNOUNCE_INTERVAL       1       // 0 in 802.1AS
#define DEFAULT_UTC_OFFSET              34
#define DEFAULT_UTC_VALID               false
#define DEFAULT_PDELAYREQ_INTERVAL      1       // -4 in 802.1AS
#define DEFAULT_DELAYREQ_INTERVAL       3       // From DEFAULT_SYNC_INTERVAL to DEFAULT_SYNC_INTERVAL + 5.
#define DEFAULT_SYNC_INTERVAL           0       // -7 in 802.1AS
#define DEFAULT_SYNC_RECEIPT_TIMEOUT    3
#define DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT 3      // 3 by default
#define DEFAULT_QUALIFICATION_TIMEOUT   -9      // DEFAULT_ANNOUNCE_INTERVAL + N
#define DEFAULT_FOREIGN_MASTER_TIME_WINDOW 4
#define DEFAULT_FOREIGN_MASTER_THRESHOLD 2
#define DEFAULT_CLOCK_CLASS             248
#define DEFAULT_CLOCK_CLASS_SLAVE_ONLY  255
#define DEFAULT_CLOCK_ACCURACY          0xFE
#define DEFAULT_PRIORITY1               248
#define DEFAULT_PRIORITY2               248
#define DEFAULT_CLOCK_VARIANCE          5000    // To be determined in 802.1AS.
#define DEFAULT_MAX_FOREIGN_RECORDS     5
#define DEFAULT_PARENTS_STATS           false
#define DEFAULT_TWO_STEP_FLAG           true    // Transmitting only SYNC message or SYNC and FOLLOW UP.
#define DEFAULT_TIME_SOURCE             GPS
#define DEFAULT_TIME_TRACEABLE          false   // Time derived from atomic clock?
#define DEFAULT_FREQUENCY_TRACEABLE     false   // Frequency derived from frequency standard?
#define DEFAULT_TIMESCALE               ARB_TIMESCALE // PTP_TIMESCALE or ARB_TIMESCALE

#define DEFAULT_CALIBRATED_OFFSET_NS    10000     // Offset from master < 10us -> calibrated
#define DEFAULT_UNCALIBRATED_OFFSET_NS  1000000   // Offset from master > 1000us -> uncalibrated
#define MAX_ADJ_OFFSET_NS               100000000  // Max offset to try to adjust it < 100ms

// Features, only change to refelect changes in implementation.
#define NUMBER_PORTS                    1
#define VERSION_PTP                     2
#define BOUNDARY_CLOCK                  false
#define SLAVE_ONLY                      true
#define NO_ADJUST                       false

// Minimal length values for each message.
// If TLV used length could be higher.
#define HEADER_LENGTH                   34
#define ANNOUNCE_LENGTH                 64
#define SYNC_LENGTH                     44
#define FOLLOW_UP_LENGTH                44
#define PDELAY_REQ_LENGTH               54
#define DELAY_REQ_LENGTH                44
#define DELAY_RESP_LENGTH               54
#define PDELAY_RESP_LENGTH              54
#define PDELAY_RESP_FOLLOW_UP_LENGTH    54
#define MANAGEMENT_LENGTH               48

//
// Enumerations defined in tables of the spec.
//

// Domain Number (Table 2 in the spec).
enum
{
  DFLT_DOMAIN_NUMBER = 0,
  ALT1_DOMAIN_NUMBER,
  ALT2_DOMAIN_NUMBER,
  ALT3_DOMAIN_NUMBER
};

// Network Protocol  (Table 3 in the spec).
enum
{
  UDP_IPV4 = 1,
  UDP_IPV6,
  IEE_802_3,
  DeviceNet,
  ControlNet,
  PROFINET
};

// Time Source (Table 7 in the spec).
enum
{
  ATOMIC_CLOCK = 0x10,
  GPS = 0x20,
  TERRESTRIAL_RADIO = 0x30,
  PTP = 0x40,
  NTP = 0x50,
  HAND_SET = 0x60,
  OTHER = 0x90,
  INTERNAL_OSCILLATOR = 0xA0
};


// PTP State (Table 8 in the spec).
enum
{
  PTP_INITIALIZING = 0,
  PTP_FAULTY,
  PTP_DISABLED,
  PTP_LISTENING,
  PTP_PRE_MASTER,
  PTP_MASTER,
  PTP_PASSIVE,
  PTP_UNCALIBRATED,
  PTP_SLAVE
};

// Delay mechanism (Table 9 in the spec).
enum
{
  E2E = 1,
  P2P = 2,
  DELAY_DISABLED = 0xFE
};

// PTP timers.
enum
{
  PDELAYREQ_INTERVAL_TIMER = 0,   // Timer handling the PdelayReq Interval
  DELAYREQ_INTERVAL_TIMER,        // Timer handling the delayReq Interval
  SYNC_INTERVAL_TIMER,            // Timer handling Interval between master sends two Syncs messages
  ANNOUNCE_RECEIPT_TIMER,         // Timer handling announce receipt timeout
  ANNOUNCE_INTERVAL_TIMER,        // Timer handling interval before master sends two announce messages
  QUALIFICATION_TIMEOUT,
  TIMER_ARRAY_SIZE                // This one is non-spec
};

// PTP Messages (Table 19).
enum
{
  SYNC = 0x0,
  DELAY_REQ,
  PDELAY_REQ,
  PDELAY_RESP,
  FOLLOW_UP = 0x8,
  DELAY_RESP,
  PDELAY_RESP_FOLLOW_UP,
  ANNOUNCE,
  SIGNALING,
  MANAGEMENT,
};

// PTP Messages control field (Table 23).
enum
{
  CTRL_SYNC = 0x00,
  CTRL_DELAY_REQ,
  CTRL_FOLLOW_UP,
  CTRL_DELAY_RESP,
  CTRL_MANAGEMENT,
  CTRL_OTHER,
};

// Output statistics.
enum
{
  PTP_NO_STATS = 0,
  PTP_TEXT_STATS,
  PTP_CSV_STATS // Not implemented.
};

// Message flags.
enum
{
  FLAG0_ALTERNATE_MASTER = 0x01,
  FLAG0_TWO_STEP = 0x02,
  FLAG0_UNICAST = 0x04,
  FLAG0_PTP_PROFILE_SPECIFIC_1 = 0x20,
  FLAG0_PTP_PROFILE_SPECIFIC_2 = 0x40,
  FLAG0_SECURITY = 0x80,
};

// Message flags.
enum
{
  FLAG1_LEAP61 = 0x01,
  FLAG1_LEAP59 = 0x02,
  FLAG1_UTC_OFFSET_VALID = 0x04,
  FLAG1_PTP_TIMESCALE = 0x08,
  FLAG1_TIME_TRACEABLE = 0x10,
  FLAG1_FREQUENCY_TRACEABLE = 0x20,
};

// PTP stack events.
enum
{
  POWERUP = 0x0001,
  INITIALIZE = 0x0002,
  DESIGNATED_ENABLED = 0x0004,
  DESIGNATED_DISABLED = 0x0008,
  FAULT_CLEARED = 0x0010,
  FAULT_DETECTED = 0x0020,
  STATE_DECISION_EVENT = 0x0040,
  QUALIFICATION_TIMEOUT_EXPIRES = 0x0080,
  ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES = 0x0100,
  SYNCHRONIZATION_FAULT = 0x0200,
  MASTER_CLOCK_SELECTED = 0x0400,
  MASTER_CLOCK_CHANGED = 0x0800 // Non-spec.
};

// PTP time scale.
enum
{
  ARB_TIMESCALE,
  PTP_TIMESCALE
};

// Platform-dependent constants definitions.

#define IF_NAMESIZE                 2
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN             16
#endif

#define IFACE_NAME_LENGTH           IF_NAMESIZE
#define NET_ADDRESS_LENGTH          INET_ADDRSTRLEN

// pow2ms(a) = round(pow(2,a) * 1000)
#define pow2ms(a) (((a)>0) ? (1000 << (a)) : (1000 >>(-(a))))

#define ADJ_FREQ_MAX  5120000

// UDP/IPv4 dependent.
#define SUBDOMAIN_ADDRESS_LENGTH    4
#define PORT_ADDRESS_LENGTH         2
#define PTP_UUID_LENGTH             NETIF_MAX_HWADDR_LEN
#define CLOCK_IDENTITY_LENGTH       8
#define FLAG_FIELD_LENGTH           2

// ptpdv1 value kept because of use of TLV...
#define PACKET_SIZE                 300

#define PTP_EVENT_PORT              319
#define PTP_GENERAL_PORT            320

#define DEFAULT_PTP_DOMAIN_ADDRESS  "224.0.1.129"
#define PEER_PTP_DOMAIN_ADDRESS     "224.0.0.107"

#define MM_STARTING_BOUNDARY_HOPS   0x7fff

// Must be a power of 2.
#define PBUF_QUEUE_SIZE             4
#define PBUF_QUEUE_MASK             (PBUF_QUEUE_SIZE - 1)

#ifdef __cplusplus
}
#endif

#endif /* CONSTANTS_H_*/
