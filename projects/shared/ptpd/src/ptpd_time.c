#include "ptpd.h"
#include "random.h"
#include "ethernetif.h"

#if LWIP_PTPD

uint32_t ptpd_get_rand(uint32_t rand_max)
{
  return random_get() % rand_max;
}

#if defined(STM32F4) || defined(STM32F7)
void ptpd_get_time(TimeInternal *time)
{
  ptptime_t ts;

  ethptp_get_time(&ts);
  time->seconds = ts.tv_sec;
  time->nanoseconds = ts.tv_nsec;
}

void ptpd_set_time(const TimeInternal *time)
{
  ptptime_t ts;

  DBG("resetting system clock to %d sec %d nsec\n", time->seconds, time->nanoseconds);

  ts.tv_sec = time->seconds;
  ts.tv_nsec = time->nanoseconds;
  ethptp_set_time(&ts);
}

bool ptpd_adj_freq(int32_t adj)
{
  DBGV("ptpd_adj_freq %d\n", adj);

  if (adj > ADJ_FREQ_MAX)
    adj = ADJ_FREQ_MAX;
  else if (adj < -ADJ_FREQ_MAX)
    adj = -ADJ_FREQ_MAX;

  /* Fine update method */
  ethptp_adj_freq(adj);

  return true;
}
#endif

#if defined(CPU_MKV58F1M0VLQ24)
void ptpd_get_time(TimeInternal *time)
{
  enet_ptp_time_t ptp_time;

  ethptp_get_time(netif_default, &ptp_time);
  time->seconds = ptp_time.second;
  time->nanoseconds = ptp_time.nanosecond;
}

void ptpd_set_time(const TimeInternal *time)
{
  enet_ptp_time_t ptp_time;

  DBG("ptpd_set_time setting system clock to %d sec %d nsec\n", time->seconds, time->nanoseconds);

  ptp_time.second = time->seconds;
  ptp_time.nanosecond = time->nanoseconds;
  ethptp_set_time(netif_default, &ptp_time);
}

bool ptpd_adj_freq(int32_t adj)
{
  DBGV("ptpd_adj_freq %d\n", adj);

  if (adj > ADJ_FREQ_MAX)
    adj = ADJ_FREQ_MAX;
  else if (adj < -ADJ_FREQ_MAX)
    adj = -ADJ_FREQ_MAX;

  /* Fine update method */
  ethptp_adj_freq(netif_default, adj);

  return true;
}
#endif

#endif // LWIP_PTPD

