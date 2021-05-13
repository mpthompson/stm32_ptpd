#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(USE_WATCHDOG)
#define USE_WATCHDOG      1
#endif

// Enumeration for times that specify the watchdog timeout.
typedef enum
{
  WATCHDOG_TIMEOUT_5MS = 0X00,
  WATCHDOG_TIMEOUT_10MS = 0X01,
  WATCHDOG_TIMEOUT_15MS = 0X02,
  WATCHDOG_TIMEOUT_30MS = 0X03,
  WATCHDOG_TIMEOUT_60MS = 0X04,
  WATCHDOG_TIMEOUT_120MS = 0X05,
  WATCHDOG_TIMEOUT_250MS = 0X06,
  WATCHDOG_TIMEOUT_500MS = 0X07,
  WATCHDOG_TIMEOUT_1S = 0X08,
  WATCHDOG_TIMEOUT_2S = 0X09,
  WATCHDOG_TIMEOUT_4S = 0X0A,
  WATCHDOG_TIMEOUT_8S = 0X0B,
  WATCHDOG_TIMEOUT_16S = 0X0C,
  WATCHDOG_TIMEOUT_32S = 0X0D
} watchdog_timeout_t;


void watchdog_init(void);
void watchdog_start(watchdog_timeout_t timeout);
void watchdog_kick(void);

#ifdef __cplusplus
}
#endif

#endif /* __WATCHDOG_H__ */
