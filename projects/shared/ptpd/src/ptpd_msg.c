#include <string.h>
#include "ptpd.h"

#if LWIP_PTPD

// Unpack header message.
void ptpd_msg_unpack_header(const octet_t *buf, MsgHeader *header)
{
  int32_t msb;
  uint32_t lsb;

  header->transportSpecific = (*(nibble_t*)(buf + 0)) >> 4;
  header->messageType = (*(enum4bit_t*)(buf + 0)) & 0x0F;
  header->versionPTP = (*(uint4bit_t*)(buf  + 1)) & 0x0F; // Force reserved bit to zero if not.
  header->messageLength = flip16(*(int16_t*)(buf  + 2));
  header->domainNumber = (*(uint8_t*)(buf + 4));
  memcpy(header->flagField, (buf + 6), FLAG_FIELD_LENGTH);
  memcpy(&msb, (buf + 8), 4);
  memcpy(&lsb, (buf + 12), 4);
  header->correctionfield = flip32(msb);
  header->correctionfield <<= 32;
  header->correctionfield += flip32(lsb);
  memcpy(header->sourcePortIdentity.clockIdentity, (buf + 20), CLOCK_IDENTITY_LENGTH);
  header->sourcePortIdentity.portNumber = flip16(*(int16_t*)(buf  + 28));
  header->sequenceId = flip16(*(int16_t*)(buf + 30));
  header->controlField = (*(uint8_t*)(buf + 32));
  header->logMessageInterval = (*(int8_t*)(buf + 33));
}

// Pack header message.
void ptpd_msg_pack_header(const PtpClock *ptp_clock, octet_t *buf)
{
  nibble_t transport = 0x80; // (spec annex D)
  *(uint8_t*)(buf + 0) = transport;
  *(uint4bit_t*)(buf  + 1) = ptp_clock->portDS.versionNumber;
  *(uint8_t*)(buf + 4) = ptp_clock->defaultDS.domainNumber;
  if (ptp_clock->defaultDS.twoStepFlag)
  {
      *(uint8_t*)(buf + 6) = FLAG0_TWO_STEP;
  }
  memset((buf + 8), 0, 8);
  memcpy((buf + 20), ptp_clock->portDS.portIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH);
  *(int16_t*)(buf + 28) = flip16(ptp_clock->portDS.portIdentity.portNumber);
  *(uint8_t*)(buf + 33) = 0x7F; //Default value (spec Table 24)
}

// Pack Announce message.
void ptpd_msg_pack_announce(const PtpClock *ptp_clock, octet_t *buf)
{
  // Changes in header
  *(char*)(buf + 0) = *(char*)(buf + 0) & 0xF0; // RAZ messageType
  *(char*)(buf + 0) = *(char*)(buf + 0) | ANNOUNCE; //Table 19
  *(int16_t*)(buf + 2)  = flip16(ANNOUNCE_LENGTH);
  *(int16_t*)(buf + 30) = flip16(ptp_clock->sentAnnounceSequenceId);
  *(uint8_t*)(buf + 32) = CTRL_OTHER; // Table 23 - controlField
  *(int8_t*)(buf + 33) = ptp_clock->portDS.logAnnounceInterval;

  // Announce message
  memset((buf + 34), 0, 10); // origin_timestamp
  *(int16_t*)(buf + 44) = flip16(ptp_clock->timePropertiesDS.currentUtcOffset);
  *(uint8_t*)(buf + 47) = ptp_clock->parentDS.grandmasterPriority1;
  *(uint8_t*)(buf + 48) = ptp_clock->defaultDS.clockQuality.clockClass;
  *(enum8bit_t*)(buf + 49) = ptp_clock->defaultDS.clockQuality.clockAccuracy;
  *(int16_t*)(buf + 50) = flip16(ptp_clock->defaultDS.clockQuality.offsetScaledLogVariance);
  *(uint8_t*)(buf + 52) = ptp_clock->parentDS.grandmasterPriority2;
  memcpy((buf + 53), ptp_clock->parentDS.grandmasterIdentity, CLOCK_IDENTITY_LENGTH);
  *(int16_t*)(buf + 61) = flip16(ptp_clock->currentDS.stepsRemoved);
  *(enum8bit_t*)(buf + 63) = ptp_clock->timePropertiesDS.timeSource;
}

// Unpack Announce message.
void ptpd_msg_unpack_announce(const octet_t *buf, MsgAnnounce *announce)
{
  announce->originTimestamp.secondsField.msb = flip16(*(int16_t*)(buf + 34));
  announce->originTimestamp.secondsField.lsb = flip32(*(uint32_t*)(buf + 36));
  announce->originTimestamp.nanosecondsField = flip32(*(uint32_t*)(buf + 40));
  announce->currentUtcOffset = flip16(*(int16_t*)(buf + 44));
  announce->grandmasterPriority1 = *(uint8_t*)(buf + 47);
  announce->grandmasterClockQuality.clockClass = *(uint8_t*)(buf + 48);
  announce->grandmasterClockQuality.clockAccuracy = *(enum8bit_t*)(buf + 49);
  announce->grandmasterClockQuality.offsetScaledLogVariance = flip16(*(int16_t*)(buf  + 50));
  announce->grandmasterPriority2 = *(uint8_t*)(buf + 52);
  memcpy(announce->grandmasterIdentity, (buf + 53), CLOCK_IDENTITY_LENGTH);
  announce->stepsRemoved = flip16(*(int16_t*)(buf + 61));
  announce->timeSource = *(enum8bit_t*)(buf + 63);
}

// Pack Sync message.
void ptpd_msg_pack_sync(const PtpClock *ptp_clock, octet_t *buf, const Timestamp *origin_timestamp)
{
  // Changes in header
  *(char*)(buf + 0) = *(char*)(buf + 0) & 0xF0; // RAZ messageType
  *(char*)(buf + 0) = *(char*)(buf + 0) | SYNC; // Table 19
  *(int16_t*)(buf + 2)  = flip16(SYNC_LENGTH);
  *(int16_t*)(buf + 30) = flip16(ptp_clock->sentSyncSequenceId);
  *(uint8_t*)(buf + 32) = CTRL_SYNC; // Table 23
  *(int8_t*)(buf + 33) = ptp_clock->portDS.logSyncInterval;
  memset((buf + 8), 0, 8); // Correction field

  // Sync message.
  *(int16_t*)(buf + 34) = flip16(origin_timestamp->secondsField.msb);
  *(uint32_t*)(buf + 36) = flip32(origin_timestamp->secondsField.lsb);
  *(uint32_t*)(buf + 40) = flip32(origin_timestamp->nanosecondsField);
}

// Unpack Sync message.
void ptpd_msg_unpack_sync(const octet_t *buf, MsgSync *sync)
{
  sync->originTimestamp.secondsField.msb = flip16(*(int16_t*)(buf + 34));
  sync->originTimestamp.secondsField.lsb = flip32(*(uint32_t*)(buf + 36));
  sync->originTimestamp.nanosecondsField = flip32(*(uint32_t*)(buf + 40));
}

// Pack DelayReq message.
void ptpd_msg_pack_delay_req(const PtpClock *ptp_clock, octet_t *buf, const Timestamp *origin_timestamp)
{
  // Changes in header.
  *(char*)(buf + 0) = *(char*)(buf + 0) & 0xF0; // RAZ messageType
  *(char*)(buf + 0) = *(char*)(buf + 0) | DELAY_REQ; // Table 19
  *(int16_t*)(buf + 2)  = flip16(DELAY_REQ_LENGTH);
  *(int16_t*)(buf + 30) = flip16(ptp_clock->sentDelayReqSequenceId);
  *(uint8_t*)(buf + 32) = CTRL_DELAY_REQ; // Table 23
  *(int8_t*)(buf + 33) = 0x7F; // Table 24
  memset((buf + 8), 0, 8);

  // Delay_req message.
  *(int16_t*)(buf + 34) = flip16(origin_timestamp->secondsField.msb);
  *(uint32_t*)(buf + 36) = flip32(origin_timestamp->secondsField.lsb);
  *(uint32_t*)(buf + 40) = flip32(origin_timestamp->nanosecondsField);
}

// Unpack DelayReq message.
void ptpd_msg_unpack_delay_req(const octet_t *buf, MsgDelayReq *delay_req)
{
  delay_req->originTimestamp.secondsField.msb = flip16(*(int16_t*)(buf + 34));
  delay_req->originTimestamp.secondsField.lsb = flip32(*(uint32_t*)(buf + 36));
  delay_req->originTimestamp.nanosecondsField = flip32(*(uint32_t*)(buf + 40));
}

// Pack FollowUp message.
void ptpd_msg_pack_follow_up(const PtpClock *ptp_clock, octet_t*buf, const Timestamp *precise_origin_timestamp)
{
  // Changes in header.
  *(char*)(buf + 0) = *(char*)(buf + 0) & 0xF0; // RAZ messageType
  *(char*)(buf + 0) = *(char*)(buf + 0) | FOLLOW_UP; // Table 19
  *(int16_t*)(buf + 2)  = flip16(FOLLOW_UP_LENGTH);
  // sentSyncSequenceId has already been incremented in issueSync.
  *(int16_t*)(buf + 30) = flip16(ptp_clock->sentSyncSequenceId - 1);
  *(uint8_t*)(buf + 32) = CTRL_FOLLOW_UP; // Table 23
  *(int8_t*)(buf + 33) = ptp_clock->portDS.logSyncInterval;

  // Follow_up message.
  *(int16_t*)(buf + 34) = flip16(precise_origin_timestamp->secondsField.msb);
  *(uint32_t*)(buf + 36) = flip32(precise_origin_timestamp->secondsField.lsb);
  *(uint32_t*)(buf + 40) = flip32(precise_origin_timestamp->nanosecondsField);
}

// Unpack FollowUp message.
void ptpd_msg_unpack_follow_up(const octet_t *buf, MsgFollowUp *follow)
{
  follow->preciseOriginTimestamp.secondsField.msb = flip16(*(int16_t*)(buf  + 34));
  follow->preciseOriginTimestamp.secondsField.lsb = flip32(*(uint32_t*)(buf + 36));
  follow->preciseOriginTimestamp.nanosecondsField = flip32(*(uint32_t*)(buf + 40));
}

// Pack DelayResp message.
void ptpd_msg_pack_delay_resp(const PtpClock *ptp_clock, octet_t *buf, const MsgHeader *header, const Timestamp *receive_timestamp)
{
  // Changes in header.
  *(char*)(buf + 0) = *(char*)(buf + 0) & 0xF0; // RAZ messageType
  *(char*)(buf + 0) = *(char*)(buf + 0) | DELAY_RESP; // Table 19
  *(int16_t*)(buf + 2)  = flip16(DELAY_RESP_LENGTH);
  // *(uint8_t*)(buf+4) = header->domainNumber; // TODO: Why?
  memset((buf + 8), 0, 8);

  // Copy correctionField of delayReqMessage.
  *(int32_t*)(buf + 8) = flip32(header->correctionfield >> 32);
  *(int32_t*)(buf + 12) = flip32((int32_t)header->correctionfield);
  *(int16_t*)(buf + 30) = flip16(header->sequenceId);
  *(uint8_t*)(buf + 32) = CTRL_DELAY_RESP; // Table 23
  *(int8_t*)(buf + 33) = ptp_clock->portDS.logMinDelayReqInterval; //Table 24

  // delay_resp message.
  *(int16_t*)(buf + 34) = flip16(receive_timestamp->secondsField.msb);
  *(uint32_t*)(buf + 36) = flip32(receive_timestamp->secondsField.lsb);
  *(uint32_t*)(buf + 40) = flip32(receive_timestamp->nanosecondsField);
  memcpy((buf + 44), header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH);
  *(int16_t*)(buf + 52) = flip16(header->sourcePortIdentity.portNumber);
}

// Unpack DelayResp message.
void ptpd_msg_unpack_delay_resp(const octet_t *buf, MsgDelayResp *resp)
{
  resp->receiveTimestamp.secondsField.msb = flip16(*(int16_t*)(buf  + 34));
  resp->receiveTimestamp.secondsField.lsb = flip32(*(uint32_t*)(buf + 36));
  resp->receiveTimestamp.nanosecondsField = flip32(*(uint32_t*)(buf + 40));
  memcpy(resp->requestingPortIdentity.clockIdentity, (buf + 44), CLOCK_IDENTITY_LENGTH);
  resp->requestingPortIdentity.portNumber = flip16(*(int16_t*)(buf  + 52));
}

// Pack PeerDelayReq message.
void ptpd_msg_pack_peer_delay_req(const PtpClock *ptp_clock, octet_t *buf, const Timestamp *origin_timestamp)
{
  // Changes in header.
  *(char*)(buf + 0) = *(char*)(buf + 0) & 0xF0; // RAZ messageType
  *(char*)(buf + 0) = *(char*)(buf + 0) | PDELAY_REQ; // Table 19
  *(int16_t*)(buf + 2)  = flip16(PDELAY_REQ_LENGTH);
  *(int16_t*)(buf + 30) = flip16(ptp_clock->sentPDelayReqSequenceId);
  *(uint8_t*)(buf + 32) = CTRL_OTHER; // Table 23
  *(int8_t*)(buf + 33) = 0x7F; // Table 24
  memset((buf + 8), 0, 8);

  // Pdelay_req message
  *(int16_t*)(buf + 34) = flip16(origin_timestamp->secondsField.msb);
  *(uint32_t*)(buf + 36) = flip32(origin_timestamp->secondsField.lsb);
  *(uint32_t*)(buf + 40) = flip32(origin_timestamp->nanosecondsField);
  memset((buf + 44), 0, 10); // RAZ reserved octets.
}

// Unpack PeerDelayReq message.
void ptpd_msg_unpack_peer_delay_req(const octet_t *buf, MsgPDelayReq *pdelayreq)
{
  pdelayreq->originTimestamp.secondsField.msb = flip16(*(int16_t*)(buf  + 34));
  pdelayreq->originTimestamp.secondsField.lsb = flip32(*(uint32_t*)(buf + 36));
  pdelayreq->originTimestamp.nanosecondsField = flip32(*(uint32_t*)(buf + 40));
}

// Pack PeerDelayResp message.
void ptpd_msg_pack_peer_delay_resp(octet_t *buf, const MsgHeader *header, const Timestamp *request_receipt_timestamp)
{
  // Changes in header.
  *(char*)(buf + 0) = *(char*)(buf + 0) & 0xF0; // RAZ messageType
  *(char*)(buf + 0) = *(char*)(buf + 0) | PDELAY_RESP; // Table 19
  *(int16_t*)(buf + 2)  = flip16(PDELAY_RESP_LENGTH);
  // *(uint8_t*)(buf+4) = header->domainNumber; // TODO: Why?
  memset((buf + 8), 0, 8);
  *(int16_t*)(buf + 30) = flip16(header->sequenceId);
  *(uint8_t*)(buf + 32) = CTRL_OTHER; // Table 23
  *(int8_t*)(buf + 33) = 0x7F; // Table 24

  // Pdelay_resp message.
  *(int16_t*)(buf + 34) = flip16(request_receipt_timestamp->secondsField.msb);
  *(uint32_t*)(buf + 36) = flip32(request_receipt_timestamp->secondsField.lsb);
  *(uint32_t*)(buf + 40) = flip32(request_receipt_timestamp->nanosecondsField);
  memcpy((buf + 44), header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH);
  *(int16_t*)(buf + 52) = flip16(header->sourcePortIdentity.portNumber);
}

// Unpack PeerDelayResp message.
void ptpd_msg_unpack_peer_delay_resp(const octet_t *buf, MsgPDelayResp *presp)
{
  presp->requestReceiptTimestamp.secondsField.msb = flip16(*(int16_t*)(buf  + 34));
  presp->requestReceiptTimestamp.secondsField.lsb = flip32(*(uint32_t*)(buf + 36));
  presp->requestReceiptTimestamp.nanosecondsField = flip32(*(uint32_t*)(buf + 40));
  memcpy(presp->requestingPortIdentity.clockIdentity, (buf + 44), CLOCK_IDENTITY_LENGTH);
  presp->requestingPortIdentity.portNumber = flip16(*(int16_t*)(buf + 52));
}

// Pack PeerDelayRespFollowUp message.
void ptpd_msg_pack_peer_delay_resp_follow_up(octet_t *buf, const MsgHeader *header, const Timestamp *response_origin_timestamp)
{
  // Changes in header.
  *(char*)(buf + 0) = *(char*)(buf + 0) & 0xF0; // RAZ messageType
  *(char*)(buf + 0) = *(char*)(buf + 0) | PDELAY_RESP_FOLLOW_UP; // Table 19
  *(int16_t*)(buf + 2)  = flip16(PDELAY_RESP_FOLLOW_UP_LENGTH);
  *(int16_t*)(buf + 30) = flip16(header->sequenceId);
  *(uint8_t*)(buf + 32) = CTRL_OTHER; // Table 23
  *(int8_t*)(buf + 33) = 0x7F; // Table 24

  // Copy correctionField of PdelayReqMessage.
  *(int32_t*)(buf + 8) = flip32(header->correctionfield >> 32);
  *(int32_t*)(buf + 12) = flip32((int32_t)header->correctionfield);

  // Pdelay_resp_follow_up message.
  *(int16_t*)(buf + 34) = flip16(response_origin_timestamp->secondsField.msb);
  *(uint32_t*)(buf + 36) = flip32(response_origin_timestamp->secondsField.lsb);
  *(uint32_t*)(buf + 40) = flip32(response_origin_timestamp->nanosecondsField);
  memcpy((buf + 44), header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH);
  *(int16_t*)(buf + 52) = flip16(header->sourcePortIdentity.portNumber);
}

// Unpack PeerDelayRespFollowUp message.
void ptpd_msg_unpack_peer_delay_resp_follow_up(const octet_t *buf, MsgPDelayRespFollowUp *resp_follow_up)
{
  resp_follow_up->responseOriginTimestamp.secondsField.msb = flip16(*(int16_t*)(buf  + 34));
  resp_follow_up->responseOriginTimestamp.secondsField.lsb = flip32(*(uint32_t*)(buf + 36));
  resp_follow_up->responseOriginTimestamp.nanosecondsField = flip32(*(uint32_t*)(buf + 40));
  memcpy(resp_follow_up->requestingPortIdentity.clockIdentity, (buf + 44), CLOCK_IDENTITY_LENGTH);
  resp_follow_up->requestingPortIdentity.portNumber = flip16(*(int16_t*)(buf + 52));
}

#endif // LWIP_PTPD
