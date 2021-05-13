#include "ptpd.h"
#include "syslog.h"

#if LWIP_PTPD

static void handle(PtpClock*);
static void handle_announce(PtpClock*, bool);
static void handle_sync(PtpClock*, TimeInternal*, bool);
static void handle_follow_up(PtpClock*, bool);
static void handle_peer_delay_req(PtpClock*, TimeInternal*, bool);
static void handle_delay_req(PtpClock*, TimeInternal*, bool);
static void handle_peer_delay_resp(PtpClock*, TimeInternal*, bool);
static void handle_delay_resp(PtpClock*, bool);
static void handle_peer_delay_resp_follow_up(PtpClock*, bool);
static void handle_management(PtpClock*, bool);
static void handle_signaling(PtpClock*, bool);

static void issue_delay_req_timer_expired(PtpClock*);
static void issue_announce(PtpClock*);
static void issue_sync(PtpClock*);
static void issue_follow_up(PtpClock*, const TimeInternal*);
static void issue_delay_req(PtpClock*);
static void issue_delay_resp(PtpClock*, const TimeInternal*, const MsgHeader*);
static void issue_peer_delay_req(PtpClock*);
static void issue_peer_delay_resp(PtpClock*, TimeInternal*, const MsgHeader*);
static void issue_peer_delay_resp_follow_up(PtpClock*, const TimeInternal*, const MsgHeader*);

static bool ptpd_protocol_do_init(PtpClock*);

#ifdef PTPD_DBG
static char *state_string(uint8_t state)
{
  switch (state)
  {
    case PTP_INITIALIZING: return (char *) "PTP_INITIALIZING";
    case PTP_FAULTY: return (char *) "PTP_FAULTY";
    case PTP_DISABLED: return (char *) "PTP_DISABLED";
    case PTP_LISTENING: return (char *) "PTP_LISTENING";
    case PTP_PRE_MASTER: return (char *) "PTP_PRE_MASTER";
    case PTP_MASTER: return (char *) "PTP_MASTER";
    case PTP_PASSIVE: return (char *) "PTP_PASSIVE";
    case PTP_UNCALIBRATED: return (char *) "PTP_UNCALIBRATED";
    case PTP_SLAVE: return (char *) "PTP_SLAVE";
    default: break;
  }
  return (char *) "UNKNOWN";
}
#endif

// Change state of PTP stack. Perform actions required when leaving 
// 'port_state' and entering 'state'.
void ptpd_protocol_to_state(PtpClock *ptp_clock, uint8_t state)
{
  DBG("leaving state %s\n", state_string(ptp_clock->portDS.portState));

  ptp_clock->messageActivity = true;

  // Leaving state tasks.
  switch (ptp_clock->portDS.portState)
  {
    case PTP_MASTER:
      ptpd_servo_init_clock(ptp_clock);
      ptpd_timer_stop(SYNC_INTERVAL_TIMER);
      ptpd_timer_stop(ANNOUNCE_INTERVAL_TIMER);
      ptpd_timer_stop(PDELAYREQ_INTERVAL_TIMER);
      break;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:
      if (state == PTP_UNCALIBRATED || state == PTP_SLAVE)
      {
        break;
      }
      ptpd_timer_stop(ANNOUNCE_RECEIPT_TIMER);
      switch (ptp_clock->portDS.delayMechanism)
      {
        case E2E:
          ptpd_timer_stop(DELAYREQ_INTERVAL_TIMER);
          break;
        case P2P:
          ptpd_timer_stop(PDELAYREQ_INTERVAL_TIMER);
          break;
        default:
          // None.
          break;
      }
      ptpd_servo_init_clock(ptp_clock);
      break;

    case PTP_PASSIVE:
      ptpd_servo_init_clock(ptp_clock);
      ptpd_timer_stop(PDELAYREQ_INTERVAL_TIMER);
      ptpd_timer_stop(ANNOUNCE_RECEIPT_TIMER);
      break;

    case PTP_LISTENING:
      ptpd_servo_init_clock(ptp_clock);
      ptpd_timer_stop(ANNOUNCE_RECEIPT_TIMER);
      break;

    case PTP_PRE_MASTER:
      ptpd_servo_init_clock(ptp_clock);
      ptpd_timer_stop(QUALIFICATION_TIMEOUT);
      break;

    default:
      break;
  }

  DBG("entering state %s\n", state_string(state));

  // Entering state tasks.
  switch (state)
  {
    case PTP_INITIALIZING:
      ptp_clock->portDS.portState = PTP_INITIALIZING;
      ptp_clock->recommendedState = PTP_INITIALIZING;
      syslog_printf(SYSLOG_NOTICE, "PTPD: entering INITIALIZING state");
      break;

    case PTP_FAULTY:
      ptp_clock->portDS.portState = PTP_FAULTY;
      syslog_printf(SYSLOG_NOTICE, "PTPD: entering FAULTY state");
      break;

    case PTP_DISABLED:
      ptp_clock->portDS.portState = PTP_DISABLED;
      syslog_printf(SYSLOG_NOTICE, "PTPD: entering DISABLED state");
      break;

    case PTP_LISTENING:
      ptpd_timer_start(ANNOUNCE_RECEIPT_TIMER, ptp_clock->portDS.announceReceiptTimeout * 
                                               pow2ms(ptp_clock->portDS.logAnnounceInterval));
      ptp_clock->portDS.portState = PTP_LISTENING;
      ptp_clock->recommendedState = PTP_LISTENING;
      syslog_printf(SYSLOG_NOTICE, "PTPD: entering LISTENING state");
      break;

    case PTP_PRE_MASTER:
      // If you implement not ordinary clock, you can manage this code.
      // ptpd_timer_start(QUALIFICATION_TIMEOUT, pow2ms(DEFAULT_QUALIFICATION_TIMEOUT));
      // ptp_clock->portDS.portState = PTP_PRE_MASTER;
      // break;

    case PTP_MASTER:
      // It may change during slave state.
      ptp_clock->portDS.logMinDelayReqInterval = DEFAULT_DELAYREQ_INTERVAL;
      ptpd_timer_start(SYNC_INTERVAL_TIMER, pow2ms(ptp_clock->portDS.logSyncInterval));
      DBG("SYNC INTERVAL TIMER : %d \n", pow2ms(ptp_clock->portDS.logSyncInterval));
      ptpd_timer_start(ANNOUNCE_INTERVAL_TIMER, pow2ms(ptp_clock->portDS.logAnnounceInterval));
      switch (ptp_clock->portDS.delayMechanism)
      {
        case E2E:
            // None.
            break;
        case P2P:
            ptpd_timer_start(PDELAYREQ_INTERVAL_TIMER, ptpd_get_rand(pow2ms(ptp_clock->portDS.logMinPdelayReqInterval) + 1));
            break;
        default:
            break;
      }
      ptp_clock->portDS.portState = PTP_MASTER;
      syslog_printf(SYSLOG_NOTICE, "PTPD: entering MASTER state");
      break;

    case PTP_PASSIVE:
      ptpd_timer_start(ANNOUNCE_RECEIPT_TIMER, ptp_clock->portDS.announceReceiptTimeout *
                                               pow2ms(ptp_clock->portDS.logAnnounceInterval));
      if (ptp_clock->portDS.delayMechanism == P2P)
      {
        ptpd_timer_start(PDELAYREQ_INTERVAL_TIMER, ptpd_get_rand(pow2ms(ptp_clock->portDS.logMinPdelayReqInterval + 1)));
      }
      ptp_clock->portDS.portState = PTP_PASSIVE;
      syslog_printf(SYSLOG_NOTICE, "PTPD: entering PASSIVE state");
      break;

    case PTP_UNCALIBRATED:
      ptpd_timer_start(ANNOUNCE_RECEIPT_TIMER, ptp_clock->portDS.announceReceiptTimeout *
                                               pow2ms(ptp_clock->portDS.logAnnounceInterval));
      switch (ptp_clock->portDS.delayMechanism)
      {
        case E2E:
            ptpd_timer_start(DELAYREQ_INTERVAL_TIMER, ptpd_get_rand(pow2ms(ptp_clock->portDS.logMinDelayReqInterval + 1)));
            break;
        case P2P:
            ptpd_timer_start(PDELAYREQ_INTERVAL_TIMER, ptpd_get_rand(pow2ms(ptp_clock->portDS.logMinPdelayReqInterval + 1)));
            break;
        default:
            // None.
            break;
      }
      ptp_clock->portDS.portState = PTP_UNCALIBRATED;
      syslog_printf(SYSLOG_NOTICE, "PTPD: entering UNCALIBRATED state");
      break;

    case PTP_SLAVE:
      ptp_clock->portDS.portState = PTP_SLAVE;
      syslog_printf(SYSLOG_NOTICE, "PTPD: entering SLAVE state");
      break;

    default:
      break;
  }
}

static bool ptpd_protocol_do_init(PtpClock *ptp_clock)
{
  // Initialize networking.
  ptpd_net_shutdown(&ptp_clock->netPath);

  // Initialize the network.
  if (!ptpd_net_init(&ptp_clock->netPath, ptp_clock))
  {
    ERROR("ptpd_protocol_do_init: failed to initialize network\n");
    return false;
  }
  else
  {
    // Initialize other stuff.
    ptpd_clock_init(ptp_clock);
    ptpd_timer_init();
    ptpd_servo_init_clock(ptp_clock);
    ptpd_m1(ptp_clock);
    ptpd_msg_pack_header(ptp_clock, ptp_clock->msgObuf);
    return true;
  }
}

// Run PTP stack in current state.
// Handle actions and events for 'port_state'.
void ptpd_protocol_do_state(PtpClock *ptp_clock)
{
  ptp_clock->messageActivity = false;

  switch (ptp_clock->portDS.portState)
  {
    case PTP_LISTENING:
    case PTP_UNCALIBRATED:
    case PTP_SLAVE:
    case PTP_PRE_MASTER:
    case PTP_MASTER:
    case PTP_PASSIVE:
      // State decision event.
      if (get_flag(ptp_clock->events, STATE_DECISION_EVENT))
      {
        DBGV("event STATE_DECISION_EVENT\n");
        clear_flag(ptp_clock->events, STATE_DECISION_EVENT);
        ptp_clock->recommendedState = ptpd_bmc(ptp_clock);
        DBGV("recommending state %s\n", state_string(ptp_clock->recommendedState));

        switch (ptp_clock->recommendedState)
        {
          case PTP_MASTER:
          case PTP_PASSIVE:
            if (ptp_clock->defaultDS.slaveOnly || (ptp_clock->defaultDS.clockQuality.clockClass == 255))
            {
                ptp_clock->recommendedState = PTP_LISTENING;
                DBGV("recommending state %s\n", state_string(ptp_clock->recommendedState));
            }
            break;

          default:
            break;
        }
      }
      break;

    default:
      break;
  }

  switch (ptp_clock->recommendedState)
  {
    case PTP_MASTER:
      switch (ptp_clock->portDS.portState)
      {
        case PTP_PRE_MASTER:
          if (ptpd_timer_expired(QUALIFICATION_TIMEOUT))
            ptpd_protocol_to_state(ptp_clock, PTP_MASTER);
          break;
        case PTP_MASTER:
          break;
        default:
          ptpd_protocol_to_state(ptp_clock, PTP_PRE_MASTER);
          break;
      }
      break;

    case PTP_PASSIVE:
      if (ptp_clock->portDS.portState != ptp_clock->recommendedState)
        ptpd_protocol_to_state(ptp_clock, PTP_PASSIVE);
      break;

    case PTP_SLAVE:
      switch (ptp_clock->portDS.portState)
      {
        case PTP_UNCALIBRATED:
          if (get_flag(ptp_clock->events, MASTER_CLOCK_SELECTED))
          {
            DBG("event MASTER_CLOCK_SELECTED\n");
            clear_flag(ptp_clock->events, MASTER_CLOCK_SELECTED);
            ptpd_protocol_to_state(ptp_clock, PTP_SLAVE);
          }
          if (get_flag(ptp_clock->events, MASTER_CLOCK_CHANGED))
          {
            DBG("event MASTER_CLOCK_CHANGED\n");
            clear_flag(ptp_clock->events, MASTER_CLOCK_CHANGED);
          }
          break;

        case PTP_SLAVE:
          if (get_flag(ptp_clock->events, SYNCHRONIZATION_FAULT))
          {
              DBG("event SYNCHRONIZATION_FAULT\n");
              clear_flag(ptp_clock->events, SYNCHRONIZATION_FAULT);
              ptpd_protocol_to_state(ptp_clock, PTP_UNCALIBRATED);
          }
          if (get_flag(ptp_clock->events, MASTER_CLOCK_CHANGED))
          {
              DBG("event MASTER_CLOCK_CHANGED\n");
              clear_flag(ptp_clock->events, MASTER_CLOCK_CHANGED);
              ptpd_protocol_to_state(ptp_clock, PTP_UNCALIBRATED);
          }
          break;

        default:
          ptpd_protocol_to_state(ptp_clock, PTP_UNCALIBRATED);
          break;
      }
      break;

    case PTP_LISTENING:
      if (ptp_clock->portDS.portState != ptp_clock->recommendedState)
      {
        ptpd_protocol_to_state(ptp_clock, PTP_LISTENING);
      }
      break;

    case PTP_INITIALIZING:
      break;

    default:
      DBG("ptpd_protocol_do_state: unrecognized recommended state %d\n", ptp_clock->recommendedState);
      break;
  }

  switch (ptp_clock->portDS.portState)
  {
    case PTP_INITIALIZING:
      if (ptpd_protocol_do_init(ptp_clock) == true)
      {
        ptpd_protocol_to_state(ptp_clock, PTP_LISTENING);
      }
      else
      {
        ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
      }
      break;

    case PTP_FAULTY:
      // Imaginary troubleshooting.
      DBG("event FAULT_CLEARED for state PTP_FAULT\n");
      ptpd_protocol_to_state(ptp_clock, PTP_INITIALIZING);
      return;

    case PTP_DISABLED:
      handle(ptp_clock);
      break;

    case PTP_LISTENING:
    case PTP_UNCALIBRATED:
    case PTP_SLAVE:
    case PTP_PASSIVE:
      if (ptpd_timer_expired(ANNOUNCE_RECEIPT_TIMER))
      {
        DBGV("event ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES for state %s\n",
             state_string(ptp_clock->portDS.portState));
        ptp_clock->foreignMasterDS.count = 0;
        ptp_clock->foreignMasterDS.i = 0;
        if (!(ptp_clock->defaultDS.slaveOnly || (ptp_clock->defaultDS.clockQuality.clockClass == 255)))
        {
          ptpd_m1(ptp_clock);
          ptp_clock->recommendedState = PTP_MASTER;
          DBGV("recommending state %s\n", state_string(ptp_clock->recommendedState));
          ptpd_protocol_to_state(ptp_clock, PTP_MASTER);
        }
        else if (ptp_clock->portDS.portState != PTP_LISTENING)
        {
          ptpd_protocol_to_state(ptp_clock, PTP_LISTENING);
        }
        break;
      }
      handle(ptp_clock);
      break;

    case PTP_MASTER:
      if (ptpd_timer_expired(SYNC_INTERVAL_TIMER))
      {
        DBGV("event SYNC_INTERVAL_TIMEOUT_EXPIRES for state PTP_MASTER\n");
        issue_sync(ptp_clock);
      }
      if (ptpd_timer_expired(ANNOUNCE_INTERVAL_TIMER))
      {
        DBGV("event ANNOUNCE_INTERVAL_TIMEOUT_EXPIRES for state PTP_MASTER\n");
        issue_announce(ptp_clock);
      }
      handle(ptp_clock);
      issue_delay_req_timer_expired(ptp_clock);
      break;

    default:
      DBG("ptpd_protocol_do_state: do unrecognized state %d\n", ptp_clock->portDS.portState);
      break;
  }
}

// Check and handle received messages.
static void handle(PtpClock *ptp_clock)
{
  int ret;
  bool is_from_self;
  TimeInternal time = { 0, 0 };

  if (!ptp_clock->messageActivity)
  {
    ret = ptpd_net_select(&ptp_clock->netPath, 0);
    if (ret < 0)
    {
      ERROR("handle: failed to poll sockets\n");
      ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
      return;
    }
    else if (!ret)
    {
      DBGVV("handle: nothing\n");
      return;
    }
  }

  DBGVV("handle: something\n");

  // Receive an event.
  ptp_clock->msgIbufLength = ptpd_net_recv_event(&ptp_clock->netPath, ptp_clock->msgIbuf, &time);

  // Local time is not UTC, we can calculate UTC on demand, otherwise UTC time is not used
  // time.seconds += ptp_clock->timePropertiesDS.currentUtcOffset;
  DBGV("handle: ptpd_net_recv_event returned %d\n", ptp_clock->msgIbufLength);

  if (ptp_clock->msgIbufLength < 0)
  {
    ERROR("handle: failed to receive on the event socket\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
    return;
  }
  else if (!ptp_clock->msgIbufLength)
  {
    // Receive a general packet.
    ptp_clock->msgIbufLength = ptpd_net_recv_general(&ptp_clock->netPath, ptp_clock->msgIbuf, &time);
    DBGV("handle: ptpd_net_recv_general returned %d\n", ptp_clock->msgIbufLength);
    if (ptp_clock->msgIbufLength < 0)
    {
      ERROR("handle: failed to receive on the general socket\n");
      ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
      return;
    }
    else if (!ptp_clock->msgIbufLength)
      return;
  }

  ptp_clock->messageActivity = true;

  if (ptp_clock->msgIbufLength < HEADER_LENGTH)
  {
    ERROR("handle: message shorter than header length\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
    return;
  }

  ptpd_msg_unpack_header(ptp_clock->msgIbuf, &ptp_clock->msgTmpHeader);
  DBGV("handle: unpacked message type %d\n", ptp_clock->msgTmpHeader.messageType);

  if (ptp_clock->msgTmpHeader.versionPTP != ptp_clock->portDS.versionNumber)
  {
    DBGV("handle: ignore version %d message\n", ptp_clock->msgTmpHeader.versionPTP);
    return;
  }

  if (ptp_clock->msgTmpHeader.domainNumber != ptp_clock->defaultDS.domainNumber)
  {
    DBGV("handle: ignore message from domainNumber %d\n", ptp_clock->msgTmpHeader.domainNumber);
    return;
  }

  // Spec 9.5.2.2
  is_from_self = ptpd_is_same_port_identity(&ptp_clock->portDS.portIdentity,
                                            &ptp_clock->msgTmpHeader.sourcePortIdentity);

  // Subtract the inbound latency adjustment if it is not a loop back and the
  // time stamp seems reasonable.
  if (!is_from_self && time.seconds > 0)
      ptpd_sub_time(&time, &time, &ptp_clock->inboundLatency);

  switch (ptp_clock->msgTmpHeader.messageType)
  {
    case ANNOUNCE:
      handle_announce(ptp_clock, is_from_self);
      break;

    case SYNC:
      handle_sync(ptp_clock, &time, is_from_self);
      break;

    case FOLLOW_UP:
      handle_follow_up(ptp_clock, is_from_self);
      break;

    case DELAY_REQ:
      handle_delay_req(ptp_clock, &time, is_from_self);
      break;

    case PDELAY_REQ:
      handle_peer_delay_req(ptp_clock, &time, is_from_self);
      break;

    case DELAY_RESP:
      handle_delay_resp(ptp_clock, is_from_self);
      break;

    case PDELAY_RESP:
      handle_peer_delay_resp(ptp_clock, &time, is_from_self);
      break;

    case PDELAY_RESP_FOLLOW_UP:
      handle_peer_delay_resp_follow_up(ptp_clock, is_from_self);
      break;

    case MANAGEMENT:
      handle_management(ptp_clock, is_from_self);
      break;

    case SIGNALING:
      handle_signaling(ptp_clock, is_from_self);
      break;

    default:
      DBG("handle: unrecognized message %d\n", ptp_clock->msgTmpHeader.messageType);
      break;
  }
}

// Spec 9.5.3.
static void handle_announce(PtpClock *ptp_clock, bool is_from_self)
{
  bool is_from_current_parent = false;

  DBGV("handle_announce: received in state %s\n", state_string(ptp_clock->portDS.portState));

  if (ptp_clock->msgIbufLength < ANNOUNCE_LENGTH)
  {
    ERROR("handle_announce: short message\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
    return;
  }

  if (is_from_self)
  {
    DBGV("handle_announce: ignore from self\n");
    return;
  }

  switch (ptp_clock->portDS.portState)
  {
    case PTP_INITIALIZING:
    case PTP_FAULTY:
    case PTP_DISABLED:
      DBGV("handle_announce: disreguard\n");
      break;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:
      // Valid announce message is received : BMC algorithm will be executed.
      set_flag(ptp_clock->events, STATE_DECISION_EVENT);
      is_from_current_parent = ptpd_is_same_port_identity(
                                  &ptp_clock->parentDS.parentPortIdentity,
                                  &ptp_clock->msgTmpHeader.sourcePortIdentity);
      ptpd_msg_unpack_announce(ptp_clock->msgIbuf, &ptp_clock->msgTmp.announce);
      if (is_from_current_parent)
      {
        ptpd_s1(ptp_clock, &ptp_clock->msgTmpHeader, &ptp_clock->msgTmp.announce);
        // Reset Timer handling Announce receipt timeout.
        ptpd_timer_start(ANNOUNCE_RECEIPT_TIMER, ptp_clock->portDS.announceReceiptTimeout *
                                                 pow2ms(ptp_clock->portDS.logAnnounceInterval));
      }
      else
      {
        DBGV("handle_announce: from another foreign master\n");
        // ptpd_add_foreign takes care  of AnnounceUnpacking.
        ptpd_add_foreign(ptp_clock, &ptp_clock->msgTmpHeader, &ptp_clock->msgTmp.announce);
      }
      break;

    case PTP_PASSIVE:
        ptpd_timer_start(ANNOUNCE_RECEIPT_TIMER, ptp_clock->portDS.announceReceiptTimeout *
                                                 pow2ms(ptp_clock->portDS.logAnnounceInterval));

    case PTP_MASTER:
    case PTP_PRE_MASTER:
    case PTP_LISTENING:
    default :

      DBGV("handle_announce: from another foreign master\n");
      ptpd_msg_unpack_announce(ptp_clock->msgIbuf, &ptp_clock->msgTmp.announce);

      // Valid announce message is received : BMC algorithm will be executed.
      set_flag(ptp_clock->events, STATE_DECISION_EVENT);
      ptpd_add_foreign(ptp_clock, &ptp_clock->msgTmpHeader, &ptp_clock->msgTmp.announce);

      break;
  }
}

static void handle_sync(PtpClock *ptp_clock, TimeInternal *time, bool is_from_self)
{
  TimeInternal correction_field;
  TimeInternal origin_timestamp;
  bool is_from_current_parent = false;

  DBGV("handle_sync: received in state %s\n", state_string(ptp_clock->portDS.portState));

  if (ptp_clock->msgIbufLength < SYNC_LENGTH)
  {
    ERROR("handle_sync: short message\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
    return;
  }

  switch (ptp_clock->portDS.portState)
  {
    case PTP_INITIALIZING:
    case PTP_FAULTY:
    case PTP_DISABLED:
      DBGV("handle_sync: disreguard\n");
      break;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:
      if (is_from_self)
      {
        DBGV("handle_sync: ignore from self\n");
        break;
      }
      is_from_current_parent = ptpd_is_same_port_identity(
                                  &ptp_clock->parentDS.parentPortIdentity,
                                  &ptp_clock->msgTmpHeader.sourcePortIdentity);
      if (!is_from_current_parent)
      {
        DBGV("handle_sync: ignore from another master\n");
        break;
      }
      ptp_clock->timestamp_syncRecv = *time;
      ptpd_scaled_nanoseconds_to_internal_time(&correction_field, &ptp_clock->msgTmpHeader.correctionfield);

      if (get_flag(ptp_clock->msgTmpHeader.flagField[0], FLAG0_TWO_STEP))
      {
        ptp_clock->waitingForFollowUp = true;
        ptp_clock->recvSyncSequenceId = ptp_clock->msgTmpHeader.sequenceId;
        // Save correction_field of Sync message for future use.
        ptp_clock->correctionField_sync = correction_field;
      }
      else
      {
        ptpd_msg_unpack_sync(ptp_clock->msgIbuf, &ptp_clock->msgTmp.sync);
        ptp_clock->waitingForFollowUp = false;
        // Synchronize  local clock.
        ptpd_to_internal_time(&origin_timestamp, &ptp_clock->msgTmp.sync.originTimestamp);
        // Use correction_field of Sync message for future use.
        ptpd_servo_update_offset(ptp_clock, &ptp_clock->timestamp_syncRecv, &origin_timestamp, &correction_field);
        ptpd_servo_update_clock(ptp_clock);
        issue_delay_req_timer_expired(ptp_clock);
      }
      break;

    case PTP_MASTER:
      if (!is_from_self)
      {
        DBGV("handle_sync: from another master\n");
        break;
      }
      else
      {
        DBGV("handle_sync: ignore from self\n");
        break;
      }

    case PTP_PASSIVE:
      DBGV("handle_sync: disreguard\n");
      issue_delay_req_timer_expired(ptp_clock);
      break;

    default:
      DBGV("handle_sync: disreguard\n");
      break;
  }
}

static void handle_follow_up(PtpClock *ptp_clock, bool is_from_self)
{
  TimeInternal correction_field;
  TimeInternal precise_origin_timestamp;
  bool is_from_current_parent = false;

  DBGV("handle_followup: received in state %s\n", state_string(ptp_clock->portDS.portState));

  if (ptp_clock->msgIbufLength < FOLLOW_UP_LENGTH)
  {
    ERROR("handle_followup: short message\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
    return;
  }

  if (is_from_self)
  {
    DBGV("handle_followup: ignore from self\n");
    return;
  }

  switch (ptp_clock->portDS.portState)
  {
    case PTP_INITIALIZING:
    case PTP_FAULTY:
    case PTP_DISABLED:
    case PTP_LISTENING:
      DBGV("handle_followup: disreguard\n");
      break;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:
      is_from_current_parent = ptpd_is_same_port_identity(
                                  &ptp_clock->parentDS.parentPortIdentity,
                                  &ptp_clock->msgTmpHeader.sourcePortIdentity);
      if (!ptp_clock->waitingForFollowUp)
      {
        DBGV("handle_followup: not waiting a message\n");
        break;
      }
      if (!is_from_current_parent)
      {
        DBGV("handle_followup: not from current parent\n");
        break;
      }
      if (ptp_clock->recvSyncSequenceId !=  ptp_clock->msgTmpHeader.sequenceId)
      {
        DBGV("handle_followup: SequenceID doesn't match with last Sync message\n");
        break;
      }
      ptpd_msg_unpack_follow_up(ptp_clock->msgIbuf, &ptp_clock->msgTmp.follow);
      ptp_clock->waitingForFollowUp = false;

      // Synchronize local clock.

      // Get the time the sync message was sent that this follow-up message is associated with.
      ptpd_to_internal_time(&precise_origin_timestamp, &ptp_clock->msgTmp.follow.preciseOriginTimestamp);

      // Get the correction field of the follow-up message.
      ptpd_scaled_nanoseconds_to_internal_time(&correction_field, &ptp_clock->msgTmpHeader.correctionfield);

      // Add to the correction field the correction field of the sync message.  These two correction
      // fields are combined in a single value that is passed to determine the offset from the master.
      ptpd_add_time(&correction_field, &correction_field, &ptp_clock->correctionField_sync);

      // Calculate the offset from the master as follows:
      //  <offsetFromMaster> = <sync_event_ingress_timestamp> - <precise_origin_timestamp>
      //                       - <meanPathDelay>  -  <combined_correction_field>
      // The sync_event_ingress_timestamp is the time when the slave received the sync packet.
      // Note the sync_event_ingress_timestamp is adjusted by the ptp_clock->inboundLatency.
      // The precise_origin_timestamp is the time that the master sent the sync packet.
      // The meanPathDelay is an estimate of how long it takes the packet to traverse the
      // network from the master to slave.
      // The function also applies an exponential smoothing filter to the offsetFromMaster.
      ptpd_servo_update_offset(ptp_clock, &ptp_clock->timestamp_syncRecv, &precise_origin_timestamp, &correction_field);

      // Now that we know the offset from the master, we can adjust our slave clock faster
      // or slower to bring it into alignment with the master clock.
      ptpd_servo_update_clock(ptp_clock);

      issue_delay_req_timer_expired(ptp_clock);
      break;

    case PTP_MASTER:
      DBGV("handle_followup: from another master\n");
      break;

    case PTP_PASSIVE:
      DBGV("handle_followup: disreguard\n");
      issue_delay_req_timer_expired(ptp_clock);
      break;

    default:
      DBG("handle_followup: unrecognized state\n");
      break;
  }
}

static void handle_delay_req(PtpClock *ptp_clock, TimeInternal *time, bool is_from_self)
{
  switch (ptp_clock->portDS.delayMechanism)
  {
    case E2E:
      DBGV("handle_delay_req: received in mode E2E in state %s\n", state_string(ptp_clock->portDS.portState));
      if (ptp_clock->msgIbufLength < DELAY_REQ_LENGTH)
      {
        ERROR("handle_delay_req: short message\n");
        ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
        return;
      }
      switch (ptp_clock->portDS.portState)
      {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
        case PTP_LISTENING:
          DBGV("handle_delay_req: disreguard\n");
          return;

        case PTP_SLAVE:
          DBGV("handle_delay_req: disreguard\n");
          break;

        case PTP_MASTER:
          // TODO: manage the value of ptp_clock->logMinDelayReqInterval form logSyncInterval to logSyncInterval + 5.
          issue_delay_resp(ptp_clock, time, &ptp_clock->msgTmpHeader);
          break;

        default:
          DBG("handle_delay_req: unrecognized state\n");
          break;
      }
      break;

    case P2P:
      ERROR("handle_delay_req: disreguard in P2P mode\n");
      break;

    default:
      // None.
      break;
  }
}

static void handle_delay_resp(PtpClock *ptp_clock, bool is_from_self)
{
  TimeInternal correction_field;
  bool is_current_request = false;
  bool is_from_current_parent = false;

  switch (ptp_clock->portDS.delayMechanism)
  {
    case E2E:
      DBGV("handle_delay_resp: received in mode E2E in state %s\n", state_string(ptp_clock->portDS.portState));
      if (ptp_clock->msgIbufLength < DELAY_RESP_LENGTH)
      {
        ERROR("handle_delay_resp: short message\n");
        ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
        return;
      }
      switch (ptp_clock->portDS.portState)
      {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_LISTENING:
          DBGV("handle_delay_resp: disreguard\n");
          return;

        case PTP_UNCALIBRATED:
        case PTP_SLAVE:
          ptpd_msg_unpack_delay_resp(ptp_clock->msgIbuf, &ptp_clock->msgTmp.resp);

          is_from_current_parent = ptpd_is_same_port_identity(
                                      &ptp_clock->parentDS.parentPortIdentity,
                                      &ptp_clock->msgTmpHeader.sourcePortIdentity);

          is_current_request = ptpd_is_same_port_identity(
                                    &ptp_clock->portDS.portIdentity,
                                    &ptp_clock->msgTmp.resp.requestingPortIdentity);

          if (((ptp_clock->sentDelayReqSequenceId - 1) == ptp_clock->msgTmpHeader.sequenceId) && is_current_request && is_from_current_parent)
          {
            // TODO: revisit 11.3.
            ptpd_to_internal_time(&ptp_clock->timestamp_delayReqRecv, &ptp_clock->msgTmp.resp.receiveTimestamp);

            ptpd_scaled_nanoseconds_to_internal_time(&correction_field, &ptp_clock->msgTmpHeader.correctionfield);
            ptpd_servo_update_delay(ptp_clock, &ptp_clock->timestamp_delayReqSend, &ptp_clock->timestamp_delayReqRecv, &correction_field);

            // The value of the portDS.logMinDelayReqInterval member of the data set in a multicast 
            // message, and 0x7F in a unicast message.  We assume the value is not being set if it
            // is 0x7f.  This prevents using absurdly long delays based upon this value.  More
            // more investigation is required to see if this is the proper thing to do.
            if (ptp_clock->msgTmpHeader.logMessageInterval != 0x7f)
              ptp_clock->portDS.logMinDelayReqInterval = ptp_clock->msgTmpHeader.logMessageInterval;
          }
          else
          {
            DBGV("handle_delay_resp: doesn't match with the delayReq\n");
            break;
          }
      }
      break;

    case P2P:
      ERROR("handle_delay_resp: disreguard in P2P mode\n");
      break;

    default:
      break;
  }
}


static void handle_peer_delay_req(PtpClock *ptp_clock, TimeInternal *time, bool is_from_self)
{
  switch (ptp_clock->portDS.delayMechanism)
  {
    case E2E:
      ERROR("handle_peer_delay_req: disreguard in E2E mode\n");
      break;

    case P2P:
      DBGV("handle_peer_delay_req: received in mode P2P in state %s\n", state_string(ptp_clock->portDS.portState));
      if (ptp_clock->msgIbufLength < PDELAY_REQ_LENGTH)
      {
        ERROR("handle_peer_delay_req: short message\n");
        ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
        return;
      }
      switch (ptp_clock->portDS.portState)
      {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
        case PTP_LISTENING:
          DBGV("handle_peer_delay_req: disreguard\n");
          return;

        case PTP_PASSIVE:
        case PTP_SLAVE:
        case PTP_MASTER:
          if (is_from_self)
          {
            DBGV("handle_peer_delay_req: ignore from self\n");
            break;
          }
          issue_peer_delay_resp(ptp_clock, time, &ptp_clock->msgTmpHeader);
          if ((time->seconds != 0) && get_flag(ptp_clock->msgTmpHeader.flagField[0], FLAG0_TWO_STEP))
          {
            // Not loopback mode.
            issue_peer_delay_resp_follow_up(ptp_clock, time, &ptp_clock->msgTmpHeader);
          }
          break;

        default:
          DBG("handle_peer_delay_req: unrecognized state\n");
          break;
      }
      break;

    default:
      break;
  }
}

static void handle_peer_delay_resp(PtpClock *ptp_clock, TimeInternal *time, bool is_from_self)
{
  bool is_current_request;
  TimeInternal correction_field;
  TimeInternal request_receipt_timestamp;

  switch (ptp_clock->portDS.delayMechanism)
  {
    case E2E:
      ERROR("handle_peer_delay_resp: disreguard in E2E mode\n");
      break;

    case P2P:
      DBGV("handle_peer_delay_resp: received in mode P2P in state %s\n",
           state_string(ptp_clock->portDS.portState));
      if (ptp_clock->msgIbufLength < PDELAY_RESP_LENGTH)
      {
        ERROR("handle_peer_delay_resp: short message\n");
        ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
        return;
      }
      switch (ptp_clock->portDS.portState)
      {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
        case PTP_LISTENING:
          DBGV("handle_peer_delay_resp: disreguard\n");
          return;

        case PTP_MASTER:
        case PTP_SLAVE:
          if (is_from_self)
          {
            DBGV("handle_peer_delay_resp: ignore from self\n");
            break;
          }
          ptpd_msg_unpack_peer_delay_resp(ptp_clock->msgIbuf, &ptp_clock->msgTmp.presp);
          is_current_request = ptpd_is_same_port_identity(
                                  &ptp_clock->portDS.portIdentity,
                                  &ptp_clock->msgTmp.presp.requestingPortIdentity);
          if (((ptp_clock->sentPDelayReqSequenceId - 1) == ptp_clock->msgTmpHeader.sequenceId) && is_current_request)
          {
            // Two step clock.
            if (get_flag(ptp_clock->msgTmpHeader.flagField[0], FLAG0_TWO_STEP))
            {
              ptp_clock->waitingForPDelayRespFollowUp = true;
              // Store  t4 (Fig 35).
              ptp_clock->pdelay_t4 = *time;
              // Store  t2 (Fig 35).
              ptpd_to_internal_time(&request_receipt_timestamp, &ptp_clock->msgTmp.presp.requestReceiptTimestamp);
              ptp_clock->pdelay_t2 = request_receipt_timestamp;
              ptpd_scaled_nanoseconds_to_internal_time(&correction_field, &ptp_clock->msgTmpHeader.correctionfield);
              ptp_clock->correctionField_pDelayResp = correction_field;
            }
            else
            {
              // One step clock.
              ptp_clock->waitingForPDelayRespFollowUp = false;
              // Store  t4 (Fig 35).
              ptp_clock->pdelay_t4 = *time;
              ptpd_scaled_nanoseconds_to_internal_time(&correction_field, &ptp_clock->msgTmpHeader.correctionfield);
              ptpd_servo_update_peer_delay(ptp_clock, &correction_field, false);
            }
          }
          else
          {
            DBGV("handle_peer_delay_resp: PDelayResp doesn't match with the PDelayReq.\n");
          }
          break;

        default:
          DBG("handle_peer_delay_resp: unrecognized state\n");
          break;
      }
      break;

    default:
      break;
  }
}

static void handle_peer_delay_resp_follow_up(PtpClock *ptp_clock, bool is_from_self)
{
  TimeInternal correction_field;
  TimeInternal response_origin_timestamp;

  switch (ptp_clock->portDS.delayMechanism)
  {
    case E2E:
      ERROR("handle_peer_delay_resp_follow_up: disreguard in E2E mode\n");
      break;

    case P2P:
      DBGV("handle_peer_delay_resp_follow_up: received in mode P2P in state %s\n", state_string(ptp_clock->portDS.portState));
      if (ptp_clock->msgIbufLength < PDELAY_RESP_FOLLOW_UP_LENGTH)
      {
        ERROR("handle_peer_delay_resp_follow_up: short message\n");
        ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
        return;
      }
      switch (ptp_clock->portDS.portState)
      {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
          DBGV("handle_peer_delay_resp_follow_up: disreguard\n");
          return;

        case PTP_SLAVE:
        case PTP_MASTER:
          if (!ptp_clock->waitingForPDelayRespFollowUp)
          {
            DBG("handle_peer_delay_resp_follow_up: not waiting a message\n");
            break;
          }
          if (ptp_clock->msgTmpHeader.sequenceId == ptp_clock->sentPDelayReqSequenceId - 1)
          {
            ptpd_msg_unpack_peer_delay_resp_follow_up(ptp_clock->msgIbuf, &ptp_clock->msgTmp.prespfollow);
            ptpd_to_internal_time(&response_origin_timestamp, &ptp_clock->msgTmp.prespfollow.responseOriginTimestamp);
            ptp_clock->pdelay_t3 = response_origin_timestamp;
            ptpd_scaled_nanoseconds_to_internal_time(&correction_field, &ptp_clock->msgTmpHeader.correctionfield);
            ptpd_add_time(&correction_field, &correction_field, &ptp_clock->correctionField_pDelayResp);
            ptpd_servo_update_peer_delay(ptp_clock, &correction_field, true);
            ptp_clock->waitingForPDelayRespFollowUp = false;
            break;
          }

        default:
          DBGV("handle_peer_delay_resp_follow_up: unrecognized state\n");
      }
      break;

    default:
      break;
  }
}

static void handle_management(PtpClock *ptp_clock, bool is_from_self)
{
  // Do nothing.
}

static void handle_signaling(PtpClock *ptp_clock, bool is_from_self)
{
  // Do nothing.
}

static void issue_delay_req_timer_expired(PtpClock *ptp_clock)
{
  switch (ptp_clock->portDS.delayMechanism)
  {
    case E2E:
      if (ptp_clock->portDS.portState != PTP_SLAVE)
      {
        break;
      }
      if (ptpd_timer_expired(DELAYREQ_INTERVAL_TIMER))
      {
        ptpd_timer_start(DELAYREQ_INTERVAL_TIMER, ptpd_get_rand(pow2ms(ptp_clock->portDS.logMinDelayReqInterval + 1)));
        DBGV("event DELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
        issue_delay_req(ptp_clock);
      }
      break;

    case P2P:
      if (ptpd_timer_expired(PDELAYREQ_INTERVAL_TIMER))
      {
        ptpd_timer_start(PDELAYREQ_INTERVAL_TIMER, ptpd_get_rand(pow2ms(ptp_clock->portDS.logMinPdelayReqInterval + 1)));
        DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
        issue_peer_delay_req(ptp_clock);
      }
      break;

    default:
        break;
  }
}


// Pack and send  on general multicast ip adress an Announce message.
static void issue_announce(PtpClock *ptp_clock)
{
  ptpd_msg_pack_announce(ptp_clock, ptp_clock->msgObuf);

  if (!ptpd_net_send_general(&ptp_clock->netPath, ptp_clock->msgObuf, ANNOUNCE_LENGTH))
  {
    ERROR("issue_announce: can't sent\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
  }
  else
  {
    DBGV("issue_announce\n");
    ptp_clock->sentAnnounceSequenceId++;
  }
}

// Pack and send  on event multicast ip adress a Sync message.
static void issue_sync(PtpClock *ptp_clock)
{
  Timestamp origin_timestamp;
  TimeInternal internal_time;

  // Try to predict outgoing time stamp.
  ptpd_get_time(&internal_time);
  ptpd_from_internal_time(&internal_time, &origin_timestamp);
  ptpd_msg_pack_sync(ptp_clock, ptp_clock->msgObuf, &origin_timestamp);

  if (!ptpd_net_send_event(&ptp_clock->netPath, ptp_clock->msgObuf, SYNC_LENGTH, &internal_time))
  {
    ERROR("issue_sync: can't sent\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
  }
  else
  {
    DBGV("issue_sync\n");
    ptp_clock->sentSyncSequenceId++;

    // Sync TX timestamp is valid.
    if ((internal_time.seconds != 0) && (ptp_clock->defaultDS.twoStepFlag))
    {
      ptpd_add_time(&internal_time, &internal_time, &ptp_clock->outboundLatency);
      issue_follow_up(ptp_clock, &internal_time);
    }
  }
}

// Pack and send on general multicast ip adress a FollowUp message.
static void issue_follow_up(PtpClock *ptp_clock, const TimeInternal *time)
{
  Timestamp precise_origin_timestamp;

  ptpd_from_internal_time(time, &precise_origin_timestamp);
  ptpd_msg_pack_follow_up(ptp_clock, ptp_clock->msgObuf, &precise_origin_timestamp);

  if (!ptpd_net_send_general(&ptp_clock->netPath, ptp_clock->msgObuf, FOLLOW_UP_LENGTH))
  {
    ERROR("issue_follow_up: can't sent\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
  }
  else
  {
    DBGV("issue_follow_up\n");
  }
}


// Pack and send on event multicast ip address a DelayReq message.
static void issue_delay_req(PtpClock *ptp_clock)
{
  Timestamp origin_timestamp;
  TimeInternal internal_time;

  ptpd_get_time(&internal_time);
  ptpd_from_internal_time(&internal_time, &origin_timestamp);

  ptpd_msg_pack_delay_req(ptp_clock, ptp_clock->msgObuf, &origin_timestamp);

  if (!ptpd_net_send_event(&ptp_clock->netPath, ptp_clock->msgObuf, DELAY_REQ_LENGTH, &internal_time))
  {
    ERROR("issue_delay_req: can't sent\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
  }
  else
  {
    DBGV("issue_delay_req:\n");
    ptp_clock->sentDelayReqSequenceId++;

    // Delay req TX timestamp is valid.
    if (internal_time.seconds != 0)
    {
      ptpd_add_time(&internal_time, &internal_time, &ptp_clock->outboundLatency);
      ptp_clock->timestamp_delayReqSend = internal_time;
    }
    else
    {
      DBGV("issue_delay_req: internal time invalid\n");
    }
  }
}

// Pack and send on event multicast ip adress a PDelayReq message.
static void issue_peer_delay_req(PtpClock *ptp_clock)
{
  Timestamp origin_timestamp;
  TimeInternal internal_time;

  ptpd_get_time(&internal_time);
  ptpd_from_internal_time(&internal_time, &origin_timestamp);

  ptpd_msg_pack_peer_delay_req(ptp_clock, ptp_clock->msgObuf, &origin_timestamp);

  if (!ptpd_net_send_peer_event(&ptp_clock->netPath, ptp_clock->msgObuf, PDELAY_REQ_LENGTH, &internal_time))
  {
    ERROR("issue_peer_delay_req: can't sent\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
  }
  else
  {
    DBGV("issue_peer_delay_req\n");
    ptp_clock->sentPDelayReqSequenceId++;

    // Delay req TX timestamp is valid.
    if (internal_time.seconds != 0)
    {
      ptpd_add_time(&internal_time, &internal_time, &ptp_clock->outboundLatency);
      ptp_clock->pdelay_t1 = internal_time;
    }
  }
}

// Pack and send on event multicast ip adress a PDelayResp message.
static void issue_peer_delay_resp(PtpClock *ptp_clock, TimeInternal *time, const MsgHeader *delay_req_header)
{
  Timestamp request_receipt_timestamp;

  ptpd_from_internal_time(time, &request_receipt_timestamp);
  ptpd_msg_pack_peer_delay_resp(ptp_clock->msgObuf, delay_req_header, &request_receipt_timestamp);

  if (!ptpd_net_send_peer_event(&ptp_clock->netPath, ptp_clock->msgObuf, PDELAY_RESP_LENGTH, time))
  {
    ERROR("issue_peer_delay_resp: can't sent\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
  }
  else
  {
    if (time->seconds != 0)
    {
      // Add  latency.
      ptpd_add_time(time, time, &ptp_clock->outboundLatency);
    }

    DBGV("issue_peer_delay_resp\n");
  }
}


// Pack and send on event multicast ip adress a DelayResp message.
static void issue_delay_resp(PtpClock *ptp_clock, const TimeInternal *time, const MsgHeader * delayReqHeader)
{
  Timestamp request_receipt_timestamp;

  ptpd_from_internal_time(time, &request_receipt_timestamp);
  ptpd_msg_pack_delay_resp(ptp_clock, ptp_clock->msgObuf, delayReqHeader, &request_receipt_timestamp);

  if (!ptpd_net_send_general(&ptp_clock->netPath, ptp_clock->msgObuf, PDELAY_RESP_LENGTH))
  {
    ERROR("issue_delay_resp: can't sent\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
  }
  else
  {
    DBGV("issue_delay_resp\n");
  }
}

static void issue_peer_delay_resp_follow_up(PtpClock *ptp_clock, const TimeInternal *time, const MsgHeader *delay_req_header)
{
  Timestamp response_origin_timestamp;

  ptpd_from_internal_time(time, &response_origin_timestamp);
  ptpd_msg_pack_peer_delay_resp_follow_up(ptp_clock->msgObuf, delay_req_header, &response_origin_timestamp);

  if (!ptpd_net_send_peer_general(&ptp_clock->netPath, ptp_clock->msgObuf, PDELAY_RESP_FOLLOW_UP_LENGTH))
  {
    ERROR("issue_peer_delay_resp_follow_up: can't sent\n");
    ptpd_protocol_to_state(ptp_clock, PTP_FAULTY);
  }
  else
  {
    DBGV("issue_peer_delay_resp_follow_up\n");
  }
}

#endif // LWIP_PTPD
