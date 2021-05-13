#include <stdlib.h>
#include "ptpd.h"
#include "systime.h"
#include "syslog.h"

#if LWIP_PTPD

void ptpd_servo_init_clock(PtpClock *ptp_clock)
{
  DBG("ptpd_servo_init_clock\n");

  // Clear the time.
  ptp_clock->Tms.seconds = 0;
  ptp_clock->Tms.nanoseconds = 0;

  // Clears clock servo accumulator (the I term).
  ptp_clock->observedDrift = 0;

  // One way delay.
  ptp_clock->owd_filt.n = 0;
  ptp_clock->owd_filt.s = ptp_clock->servo.sDelay;

  // Offset from master.
  ptp_clock->ofm_filt.n = 0;
  ptp_clock->ofm_filt.s = ptp_clock->servo.sOffset;

  // Scaled log variance.
  if (DEFAULT_PARENTS_STATS)
  {
    ptp_clock->slv_filt.n = 0;
    ptp_clock->slv_filt.s = 6;
    ptp_clock->offsetHistory[0] = 0;
    ptp_clock->offsetHistory[1] = 0;
  }

  // Clear the wait flags.
  ptp_clock->waitingForFollowUp = false;
  ptp_clock->waitingForPDelayRespFollowUp = false;

  // Clear the peer delays.
  ptp_clock->pdelay_t1.seconds = ptp_clock->pdelay_t1.nanoseconds = 0;
  ptp_clock->pdelay_t2.seconds = ptp_clock->pdelay_t2.nanoseconds = 0;
  ptp_clock->pdelay_t3.seconds = ptp_clock->pdelay_t3.nanoseconds = 0;
  ptp_clock->pdelay_t4.seconds = ptp_clock->pdelay_t4.nanoseconds = 0;

  // Reset parent statistics.
  ptp_clock->parentDS.parentStats = false;
  ptp_clock->parentDS.observedParentClockPhaseChangeRate = 0;
  ptp_clock->parentDS.observedParentOffsetScaledLogVariance = 0;

  // Level clock.
  if (!ptp_clock->servo.noAdjust)
    ptpd_adj_freq(0);

  // Empty the event queue.
  ptpd_net_empty_event_queue(&ptp_clock->netPath);
}

static int32_t ptpd_servo_order(int32_t n)
{
  if (n < 0) {
    n = -n;
  }
  if (n == 0) {
    return 0;
  }
  return ptpd_floor_log2(n);
}

// Exponential smoothing.
static void ptpd_servo_filter(int32_t *nsec_current, Filter *filter)
{
  int32_t s, s2;

  // using floatingpoint math
  // alpha = 1/2^s
  // y[1] = x[0]
  // y[n] = alpha * x[n-1] + (1-alpha) * y[n-1]
  //
  // or equivalent with integer math
  //
  // y[1] = x[0]
  // y_sum[1] = y[1] * 2^s
  // y_sum[n] = y_sum[n-1] + x[n-1] - y[n-1]
  // y[n] = y_sum[n] / 2^s

  // Increment number of samples.
  filter->n++;

  // If it is first time, we are running filter, initialize it.
  if (filter->n == 1)
  {
    filter->y_prev = *nsec_current;
    filter->y_sum = *nsec_current;
    filter->s_prev = 0;
  }

  s = filter->s;

  // Speedup filter, if not 2^s > n.
  if ((1 << s) > filter->n)
  {
    // Lower the filter order.
    s = ptpd_servo_order(filter->n);
  }
  else
  {
    // Avoid overflowing of n.
    filter->n = 1 << s;
  }

  // Avoid overflowing of filter. 30 is because using signed 32bit integers.
  s2 = 30 - ptpd_servo_order(max(filter->y_prev, *nsec_current));

  // Use the lower filter order, higher will overflow.
  s = min(s, s2);

  // If the order of the filter changed, change also y_sum value.
  if (filter->s_prev > s)
  {
    filter->y_sum >>= (filter->s_prev - s);
  }
  else if (filter->s_prev < s)
  {
    filter->y_sum <<= (s - filter->s_prev);
  }

  // Compute the filter itself.
  filter->y_sum += *nsec_current - filter->y_prev;
  filter->y_prev = filter->y_sum >> s;

  // Save previous order of the filter.
  filter->s_prev = s;

  DBGV("PTPD: filter: %d -> %d (%d)\n", *nsec_current, filter->y_prev, s);

  // Actualize target value.
  *nsec_current = filter->y_prev;
}

// 11.2
void ptpd_servo_update_offset(PtpClock *ptp_clock, const TimeInternal *sync_event_ingress_timestamp,
                  const TimeInternal *precise_origin_timestamp, const TimeInternal *correction_field)
{
  DBGV("ptpd_servo_update_offset\n");

  //  <offsetFromMaster> = <sync_event_ingress_timestamp> - <precise_origin_timestamp>
  //                       - <meanPathDelay>  -  correction_field  of  Sync  message
  //                       -  correction_field  of  Follow_Up message.

#if 0
  DBGVV("ptpd_servo_update_offset: ingress_timestamp %d seconds %d nanoseconds\n",
        sync_event_ingress_timestamp->seconds,
        sync_event_ingress_timestamp->nanoseconds);
  DBGVV("ptpd_servo_update_offset: origin_timestamp %d seconds %d nanoseconds\n",
        precise_origin_timestamp->seconds,
        precise_origin_timestamp->nanoseconds);
  DBGVV("ptpd_servo_update_offset: correction_field %d seconds %d nanoseconds\n",
        correction_field->seconds,
        correction_field->nanoseconds);
#endif

  // Compute offsetFromMaster.
  ptpd_sub_time(&ptp_clock->Tms, sync_event_ingress_timestamp, precise_origin_timestamp);
  ptpd_sub_time(&ptp_clock->Tms, &ptp_clock->Tms, correction_field);

  ptp_clock->currentDS.offsetFromMaster = ptp_clock->Tms;

#if 0
  DBGVV("ptpd_servo_update_offset: offset %d seconds %d nanoseconds\n",
        ptp_clock->currentDS.offsetFromMaster.seconds,
        ptp_clock->currentDS.offsetFromMaster.nanoseconds);

  DBGVV("ptpd_servo_update_offset: mean_path_delay %d seconds %d nanoseconds\n",
        ptp_clock->currentDS.meanPathDelay.seconds,
        ptp_clock->currentDS.meanPathDelay.nanoseconds);
#endif

  switch (ptp_clock->portDS.delayMechanism)
  {
    case E2E:
        ptpd_sub_time(&ptp_clock->currentDS.offsetFromMaster, &ptp_clock->currentDS.offsetFromMaster, &ptp_clock->currentDS.meanPathDelay);
        break;

    case P2P:
        ptpd_sub_time(&ptp_clock->currentDS.offsetFromMaster, &ptp_clock->currentDS.offsetFromMaster, &ptp_clock->portDS.peerMeanPathDelay);
        break;

    default:
        break;
  }

  DBGVV("ptpd_servo_update_offset: offset %d seconds %d nanoseconds\n",
        ptp_clock->currentDS.offsetFromMaster.seconds,
        ptp_clock->currentDS.offsetFromMaster.nanoseconds);

  if (ptp_clock->currentDS.offsetFromMaster.seconds != 0)
  {
    if (ptp_clock->portDS.portState == PTP_SLAVE)
    {
        set_flag(ptp_clock->events, SYNCHRONIZATION_FAULT);
    }

    DBGV("ptpd_servo_update_offset: cannot filter seconds\n");

    return;
  }

  // Filter offsetFromMaster.
  ptpd_servo_filter(&ptp_clock->currentDS.offsetFromMaster.nanoseconds, &ptp_clock->ofm_filt);

  // Check results.
  if (abs(ptp_clock->currentDS.offsetFromMaster.nanoseconds) < DEFAULT_CALIBRATED_OFFSET_NS)
  {
    if (ptp_clock->portDS.portState == PTP_UNCALIBRATED)
    {
        set_flag(ptp_clock->events, MASTER_CLOCK_SELECTED);
    }
  }
  else if (abs(ptp_clock->currentDS.offsetFromMaster.nanoseconds) > DEFAULT_UNCALIBRATED_OFFSET_NS)
  {
    if (ptp_clock->portDS.portState == PTP_SLAVE)
    {
        set_flag(ptp_clock->events, SYNCHRONIZATION_FAULT);
    }
  }
}

// 11.3.
void ptpd_servo_update_delay(PtpClock * ptp_clock, const TimeInternal *delay_event_egress_timestamp,
                 const TimeInternal *receive_timestamp, const TimeInternal *correction_field)
{
  // Tms valid?
  if (ptp_clock->ofm_filt.n == 0)
  {
    DBGV("PTPD: ptpd_servo_update_delay: Tms is not valid");
    return;
  }

#if 1
  DBGVV("ptpd_servo_update_delay: receive_timestamp %d seconds %d nanoseconds\n",
        receive_timestamp->seconds,
        receive_timestamp->nanoseconds);
  DBGVV("ptpd_servo_update_delay: egress_timestamp %d seconds %d nanoseconds\n",
        delay_event_egress_timestamp->seconds,
        delay_event_egress_timestamp->nanoseconds);
  DBGVV("ptpd_servo_update_delay: correction_field %d seconds %d nanoseconds\n",
        correction_field->seconds,
        correction_field->nanoseconds);
#endif

  ptpd_sub_time(&ptp_clock->Tsm, receive_timestamp, delay_event_egress_timestamp);
  ptpd_sub_time(&ptp_clock->Tsm, &ptp_clock->Tsm, correction_field);
  ptpd_add_time(&ptp_clock->currentDS.meanPathDelay, &ptp_clock->Tms, &ptp_clock->Tsm);
  ptpd_div2_time(&ptp_clock->currentDS.meanPathDelay);

  DBGVV("ptpd_servo_update_delay: meanPathDelay %d seconds %d nanoseconds\n",
        ptp_clock->currentDS.meanPathDelay.seconds,
        ptp_clock->currentDS.meanPathDelay.nanoseconds);

  // Filter delay.
  if (ptp_clock->currentDS.meanPathDelay.seconds != 0)
  {
    DBGV("PTPD: ptpd_servo_update_delay: cannot filter with seconds\n");
  }
  else
  {
    ptpd_servo_filter(&ptp_clock->currentDS.meanPathDelay.nanoseconds, &ptp_clock->owd_filt);
  }
}

void ptpd_servo_update_peer_delay(PtpClock *ptp_clock, const TimeInternal *correction_field, bool two_step)
{
  DBGV("PTPD: ptpd_servo_update_peer_delay\n");

  if (two_step)
  {
    // Two-step clock.
    TimeInternal tab, tba;
    ptpd_sub_time(&tab, &ptp_clock->pdelay_t2 , &ptp_clock->pdelay_t1);
    ptpd_sub_time(&tba, &ptp_clock->pdelay_t4, &ptp_clock->pdelay_t3);
    ptpd_add_time(&ptp_clock->portDS.peerMeanPathDelay, &tab, &tba);
  }
  else
  {
    // One-step clock.
    ptpd_sub_time(&ptp_clock->portDS.peerMeanPathDelay, &ptp_clock->pdelay_t4, &ptp_clock->pdelay_t1);
  }

  ptpd_sub_time(&ptp_clock->portDS.peerMeanPathDelay, &ptp_clock->portDS.peerMeanPathDelay, correction_field);
  ptpd_div2_time(&ptp_clock->portDS.peerMeanPathDelay);

  // Filter delay.
  if (ptp_clock->portDS.peerMeanPathDelay.seconds != 0)
  {
    DBGV("PTPD: ptpd_servo_update_peer_delay: cannot filter with seconds");
    return;
  }
  else
  {
    ptpd_servo_filter(&ptp_clock->portDS.peerMeanPathDelay.nanoseconds, &ptp_clock->owd_filt);
  }
}

void ptpd_servo_update_clock(PtpClock *ptp_clock)
{
  int32_t adj;
  TimeInternal timeTmp;
  int32_t offsetNorm;
  char buffer[32];
  
  DBGV("PTPD: ptpd_servo_update_clock offset %d sec %d nsec\n",
       ptp_clock->currentDS.offsetFromMaster.seconds,
       abs(ptp_clock->currentDS.offsetFromMaster.nanoseconds));

  if (ptp_clock->currentDS.offsetFromMaster.seconds != 0 || abs(ptp_clock->currentDS.offsetFromMaster.nanoseconds) > MAX_ADJ_OFFSET_NS)
  {
    // If secs, reset clock or set freq adjustment to max.
    if (!ptp_clock->servo.noAdjust)
    {
      if (!ptp_clock->servo.noResetClock)
      {
        // Get the current time.
        ptpd_get_time(&timeTmp);

        // Subtract the offset from the master.
        ptpd_sub_time(&timeTmp, &timeTmp, &ptp_clock->currentDS.offsetFromMaster);

        // Set the time with the offset.
        ptpd_set_time(&timeTmp);

        // Get the date from system time.
        systime_str(buffer, sizeof(buffer));

        // Log the time being set.
        syslog_printf(SYSLOG_NOTICE, "PTPD: setting %s", buffer);

        // Reinitialize clock.
        ptpd_servo_init_clock(ptp_clock);
      }
      else
      {
        adj = ptp_clock->currentDS.offsetFromMaster.nanoseconds > 0 ? ADJ_FREQ_MAX : -ADJ_FREQ_MAX;
        ptpd_adj_freq(-adj);
      }
    }
  }
  else
  {
    // The PI controller.

    // Normalize offset to 1s sync interval -> response of the servo
    // will be same for all sync interval values, but faster/slower
    // (possible lost of precision/overflow but much more stable).
    offsetNorm = ptp_clock->currentDS.offsetFromMaster.nanoseconds;
    if (ptp_clock->portDS.logSyncInterval > 0)
      offsetNorm >>= ptp_clock->portDS.logSyncInterval;
    else if (ptp_clock->portDS.logSyncInterval < 0)
      offsetNorm <<= -ptp_clock->portDS.logSyncInterval;

    // The accumulator for the I component.
    ptp_clock->observedDrift += offsetNorm / ptp_clock->servo.ai;

    // Clamp the accumulator to ADJ_FREQ_MAX for sanity.
    if (ptp_clock->observedDrift > ADJ_FREQ_MAX)
      ptp_clock->observedDrift = ADJ_FREQ_MAX;
    else if (ptp_clock->observedDrift < -ADJ_FREQ_MAX)
      ptp_clock->observedDrift = -ADJ_FREQ_MAX;

    // Apply controller output as a clock tick rate adjustment.
    if (!ptp_clock->servo.noAdjust)
    {
      adj = offsetNorm / ptp_clock->servo.ap + ptp_clock->observedDrift;
      ptpd_adj_freq(-adj);
    }

    if (DEFAULT_PARENTS_STATS)
    {
      int32_t a, scaledLogVariance;
      ptp_clock->parentDS.parentStats = true;
      ptp_clock->parentDS.observedParentClockPhaseChangeRate = 1100 * ptp_clock->observedDrift;

      a = (ptp_clock->offsetHistory[1] - 2 * ptp_clock->offsetHistory[0] + ptp_clock->currentDS.offsetFromMaster.nanoseconds);
      ptp_clock->offsetHistory[1] = ptp_clock->offsetHistory[0];
      ptp_clock->offsetHistory[0] = ptp_clock->currentDS.offsetFromMaster.nanoseconds;

      scaledLogVariance = ptpd_servo_order(a * a) << 8;
      ptpd_servo_filter(&scaledLogVariance, &ptp_clock->slv_filt);
      ptp_clock->parentDS.observedParentOffsetScaledLogVariance = 17000 + scaledLogVariance;
      DBGV("PTPD: ptpd_servo_update_clock: observed scalled log variance: 0x%x\n", ptp_clock->parentDS.observedParentOffsetScaledLogVariance);
    }
  }

  switch (ptp_clock->portDS.delayMechanism)
  {
    case E2E:
      DBG("PTPD: ptpd_servo_update_clock: one-way delay averaged (E2E): %d sec %d nsec\n",
          ptp_clock->currentDS.meanPathDelay.seconds, ptp_clock->currentDS.meanPathDelay.nanoseconds);
      break;

    case P2P:
      DBG("PTPD: ptpd_servo_update_clock: one-way delay averaged (P2P): %d sec %d nsec\n",
          ptp_clock->portDS.peerMeanPathDelay.seconds, ptp_clock->portDS.peerMeanPathDelay.nanoseconds);
      break;

    default:
      DBG("PTPD: ptpd_servo_update_clock: one-way delay not computed\n");
  }

  DBG("PTPD: ptpd_servo_update_clock: offset from master: %d sec %d nsec\n",
      ptp_clock->currentDS.offsetFromMaster.seconds,
      ptp_clock->currentDS.offsetFromMaster.nanoseconds);
  DBG("PTPD: ptpd_servo_update_clock: observed drift: %d\n", ptp_clock->observedDrift);
}

#endif // LWIP_PTPD
