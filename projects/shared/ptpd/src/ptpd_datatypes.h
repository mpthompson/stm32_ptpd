#ifndef DATATYPES_H_
#define DATATYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "lwip/api.h"
#include "ptpd_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// Implementation specific datatypes.

// 4-bit enumeration.
typedef unsigned char enum4bit_t;

// 8-bit enumeration.
typedef unsigned char enum8bit_t;

// 16-bit enumeration.
typedef unsigned short enum16bit_t;

// 4-bit unsigned integer.
typedef unsigned char uint4bit_t; 

// 48-bit unsigned integer.
typedef struct
{
  unsigned int lsb;
  unsigned short msb;
} uint48bit_t;

// 4-bit data without numerical representation
typedef unsigned char nibble_t;

// 8-bit data without numerical representation
typedef char octet_t; 

// Struct used to average the offset from master and the one way delay.
//
// Exponencial smoothing
//
// alpha = 1/2^s
// y[1] = x[0]
// y[n] = alpha * x[n-1] + (1-alpha) * y[n-1]
//
typedef struct
{
  int32_t y_prev;
  int32_t y_sum;
  int16_t s;
  int16_t s_prev;
  int32_t n;
} Filter;

// Network buffer queue.
typedef struct
{
  void *pbuf[PBUF_QUEUE_SIZE];
  int16_t head;
  int16_t tail;
  sys_mutex_t mutex;
} BufQueue;

// Struct used to store network data.
typedef struct
{
  int32_t unicastAddr;
  int32_t multicastAddr;
  int32_t peerMulticastAddr;

  struct udp_pcb *eventPcb;
  struct udp_pcb *generalPcb;

  BufQueue eventQ;
  BufQueue generalQ;
} NetPath;

// Define compiler specific symbols.
#if defined (__GNUC__) && !defined (__ARMCC_VERSION)
#include <unistd.h>
#endif

// 5.3 Derived data type specifications
// Below are structures defined by the spec, main program data 
// structure, and all messages structures.

// 5.3.2 The TimeInterval type represents time intervals in
// scaled nanoseconds where scaledNanoseconds = time[ns] * 2^16.
typedef struct
{
  int64_t scaledNanoseconds;
} TimeInterval;

// 5.3.3 The Timestamp type represents a positive time with
// respect to the epoch.
typedef struct
{
  uint48bit_t secondsField;
  uint32_t nanosecondsField;
} Timestamp;

// 5.3.4 The ClockIdentity type identifies a clock.
typedef octet_t ClockIdentity[CLOCK_IDENTITY_LENGTH];

// 5.3.5 The PortIdentity identifies a PTP port.
typedef struct
{
  ClockIdentity clockIdentity;
  int16_t portNumber;
} PortIdentity;

// 5.3.7 The ClockQuality represents the quality of a clock.
typedef struct
{
  uint8_t clockClass;
  enum8bit_t clockAccuracy;
  int16_t offsetScaledLogVariance;
} ClockQuality;

// 5.3.8 The TLV type represents TLV extension fields.
typedef struct
{
  enum16bit_t tlvType;
  int16_t lengthField;
  octet_t* valueField;
} TLV;

// 5.3.9 The PTPText data type is used to represent textual material
// in PTP messages textField - UTF-8 encoding.
typedef struct
{
  uint8_t lengthField;
  octet_t *textField;
} PTPText;

// 5.3.10 The FaultRecord type is used to construct fault logs.
typedef struct
{
  int16_t faultRecordLength;
  Timestamp faultTime;
  enum8bit_t severityCode;
  PTPText faultName;
  PTPText faultValue;
  PTPText faultDescription;
} FaultRecord;


// The common header for all PTP messages (Table 18 of the spec).
typedef struct
{
  nibble_t transportSpecific;
  enum4bit_t messageType;
  uint4bit_t  versionPTP;
  int16_t messageLength;
  uint8_t domainNumber;
  octet_t flagField[2];
  int64_t correctionfield;
  PortIdentity sourcePortIdentity;
  int16_t sequenceId;
  uint8_t controlField;
  int8_t logMessageInterval;
} MsgHeader;

// Announce message fields (Table 25 of the spec).
typedef struct
{
  Timestamp originTimestamp;
  int16_t currentUtcOffset;
  uint8_t grandmasterPriority1;
  ClockQuality grandmasterClockQuality;
  uint8_t grandmasterPriority2;
  ClockIdentity grandmasterIdentity;
  int16_t stepsRemoved;
  enum8bit_t timeSource;
} MsgAnnounce;


// Sync message fields (Table 26 of the spec).
typedef struct
{
  Timestamp originTimestamp;
} MsgSync;

// DelayReq message fields (Table 26 of the spec).
typedef struct
{
  Timestamp originTimestamp;
} MsgDelayReq;

// DelayResp message fields (Table 30 of the spec).
typedef struct
{
  Timestamp receiveTimestamp;
  PortIdentity requestingPortIdentity;
} MsgDelayResp;

// FollowUp message fields (Table 27 of the spec).
typedef struct
{
  Timestamp preciseOriginTimestamp;
} MsgFollowUp;

// PDelayReq message fields (Table 29 of the spec).
typedef struct
{
  Timestamp originTimestamp;
} MsgPDelayReq;

// PDelayResp message fields (Table 30 of the spec).
typedef struct
{
  Timestamp requestReceiptTimestamp;
  PortIdentity requestingPortIdentity;
} MsgPDelayResp;

// PDelayRespFollowUp message fields (Table 31 of the spec).
typedef struct
{
  Timestamp responseOriginTimestamp;
  PortIdentity requestingPortIdentity;
} MsgPDelayRespFollowUp;

// Signaling message fields (Table 33 of the spec).
typedef struct
{
  PortIdentity targetPortIdentity;
  char *tlv;
} MsgSignaling;

// Management message fields (Table 37 of the spec).
typedef struct
{
  PortIdentity targetPortIdentity;
  uint8_t startingBoundaryHops;
  uint8_t boundaryHops;
  enum4bit_t actionField;
  char *tlv;
} MsgManagement;

// Time structure to handle Linux time information.
typedef struct
{
  int32_t seconds;
  int32_t nanoseconds;
} TimeInternal;

// ForeignMasterRecord is used to manage foreign masters.
typedef struct
{
  PortIdentity foreignMasterPortIdentity;
  int16_t foreignMasterAnnounceMessages;

  // This one is not in the spec.
  MsgAnnounce announce;
  MsgHeader header;
} ForeignMasterRecord;

// Spec 8.2.1 default data set.
// Spec 7.6.2, spec 7.6.3 PTP device attributes.
typedef struct
{
  bool twoStepFlag;
  ClockIdentity clockIdentity; // spec 7.6.2.1
  int16_t numberPorts;  // spec 7.6.2.7
  ClockQuality clockQuality; // spec 7.6.2.4, 7.6.3.4 and 7.6.3
  uint8_t priority1; // spec 7.6.2.2
  uint8_t priority2; // spec 7.6.2.3
  uint8_t domainNumber;
  bool slaveOnly;
} DefaultDS;

// Brief spec 8.2.2 current data set.
typedef struct
{
  int16_t stepsRemoved;
  TimeInternal offsetFromMaster;
  TimeInternal meanPathDelay;
} CurrentDS;

// Spec 8.2.3 parent data set.
typedef struct
{
  PortIdentity parentPortIdentity;
  // 7.6.4 Parent clock statistics - parentDS 
  bool  parentStats; // spec 7.6.4.2
  int16_t observedParentOffsetScaledLogVariance; // spec 7.6.4.3
  int32_t observedParentClockPhaseChangeRate; // spec 7.6.4.4
  ClockIdentity grandmasterIdentity;
  ClockQuality grandmasterClockQuality;
  uint8_t grandmasterPriority1;
  uint8_t grandmasterPriority2;
} ParentDS;

// Spec 8.2.4 time properties data set.
typedef struct
{
  int16_t currentUtcOffset;
  bool  currentUtcOffsetValid;
  bool  leap59;
  bool  leap61;
  bool  timeTraceable;
  bool  frequencyTraceable;
  bool  ptpTimescale;
  enum8bit_t timeSource; // spec 7.6.2.6
} TimePropertiesDS;

// Spec 8.2.5 port data set.
typedef struct
{
  PortIdentity portIdentity;
  enum8bit_t portState;
  int8_t logMinDelayReqInterval; // spec 7.7.2.4
  TimeInternal peerMeanPathDelay;
  int8_t logAnnounceInterval; // spec 7.7.2.2
  uint8_t announceReceiptTimeout; // spec 7.7.3.1
  int8_t logSyncInterval; // spec 7.7.2.3
  enum8bit_t delayMechanism;
  int8_t logMinPdelayReqInterval; // spec 7.7.2.5
  uint4bit_t  versionNumber;
} PortDS;

// Foreign master data set.
typedef struct
{
  ForeignMasterRecord *records;

  // Other things we need for the protocol.
  int16_t count;
  int16_t capacity;
  int16_t i;
  int16_t best;
} ForeignMasterDS;

// Clock servo filters and PI regulator values.
typedef struct
{
  bool noResetClock;
  bool noAdjust;
  int16_t ap;
  int16_t ai;
  int16_t sDelay;
  int16_t sOffset;
} Servo;

// Program options set at run-time.
typedef struct
{
  int8_t announceInterval;
  int8_t syncInterval;
  ClockQuality clockQuality;
  uint8_t priority1;
  uint8_t priority2;
  uint8_t domainNumber;
  bool slaveOnly;
  int16_t currentUtcOffset;
  octet_t ifaceName[IFACE_NAME_LENGTH];
  enum8bit_t stats;
  octet_t unicastAddress[NET_ADDRESS_LENGTH];
  TimeInternal inboundLatency;
  TimeInternal outboundLatency;
  int16_t maxForeignRecords;
  enum8bit_t delayMechanism;
  Servo servo;
} RunTimeOpts;

// Main program data structure.
typedef struct
{
  // Options.
  RunTimeOpts rtOpts;

  // Data sets.
  PortDS portDS;
  ParentDS parentDS;
  DefaultDS defaultDS;
  CurrentDS currentDS;
  ForeignMasterDS foreignMasterDS;
  TimePropertiesDS timePropertiesDS;

  // Buffer for incomming message header.
  MsgHeader msgTmpHeader;

  // Buffer for incomming message body.
  union
  {
    MsgSync sync;
    MsgFollowUp follow;
    MsgDelayReq req;
    MsgDelayResp resp;
    MsgPDelayReq preq;
    MsgPDelayResp presp;
    MsgPDelayRespFollowUp prespfollow;
    MsgManagement manage;
    MsgAnnounce announce;
    MsgSignaling signaling;
  } msgTmp;

  // Buffers for outgoing and incoming messages.
  octet_t msgObuf[PACKET_SIZE];
  octet_t msgIbuf[PACKET_SIZE];
  ssize_t msgIbufLength;

  // Time Master -> Slave.
  TimeInternal Tms;

  // Time Slave -> Master.
  TimeInternal Tsm;

  // Peer delay time t1, t2, t3 and t4.
  TimeInternal pdelay_t1;
  TimeInternal pdelay_t2;
  TimeInternal pdelay_t3;
  TimeInternal pdelay_t4;

  // Timestamps for sync, delay request send and delay request receive messages.
  TimeInternal timestamp_syncRecv;
  TimeInternal timestamp_delayReqSend;
  TimeInternal timestamp_delayReqRecv;

  // Correction field for sync/follow-up and peer delay response messages.
  TimeInternal correctionField_sync;
  TimeInternal correctionField_pDelayResp;

  int16_t sentPDelayReqSequenceId;
  int16_t sentDelayReqSequenceId;
  int16_t sentSyncSequenceId;
  int16_t sentAnnounceSequenceId;

  int16_t recvPDelayReqSequenceId;
  int16_t recvSyncSequenceId;

  // True if sync message was recieved and 2step flag is set.
  bool   waitingForFollowUp;
  // True if peer delay response message was recieved and 2step flag is set.
  bool   waitingForPDelayRespFollowUp;

  // Filters for offset from master, one way delay and scaled log variance.
  Filter ofm_filt;
  Filter owd_filt;
  Filter slv_filt;

  int16_t offsetHistory[2];
  int32_t observedDrift;

  bool  messageActivity;

  enum8bit_t recommendedState;

  // Useful to init network stuff.
  octet_t portUuidField[PTP_UUID_LENGTH];

  TimeInternal inboundLatency;
  TimeInternal outboundLatency;

  NetPath netPath;
  Servo servo;
  int32_t events;
  enum8bit_t stats;
} PtpClock;

#ifdef __cplusplus
}
#endif

#endif /* DATATYPES_H_*/
