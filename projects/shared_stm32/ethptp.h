#ifndef __ETHPTP_H__
#define __ETHPTP_H__

#include <stdint.h>
#include "hal_system.h"

#ifdef __cplusplus
extern "C" {
#endif

/**--------------------------------------------------------------------------**/
/** 
  * @brief                           Ethernet PTP defines
  */ 
/**--------------------------------------------------------------------------**/
/**
  * @}
  */

/** @defgroup ETH_PTP_time_update_method 
  * @{
  */ 
#define ETH_PTP_FineUpdate        ((uint32_t)0x00000001)  /*!< Fine Update method */
#define ETH_PTP_CoarseUpdate      ((uint32_t)0x00000000)  /*!< Coarse Update method */
#define IS_ETH_PTP_UPDATE(UPDATE) (((UPDATE) == ETH_PTP_FineUpdate) || \
                                   ((UPDATE) == ETH_PTP_CoarseUpdate))

/**
  * @}
  */ 


/** @defgroup ETH_PTP_Flags 
  * @{
  */ 
#define ETH_PTP_FLAG_TSARU        ((uint32_t)0x00000020)  /*!< Addend Register Update */
#define ETH_PTP_FLAG_TSITE        ((uint32_t)0x00000010)  /*!< Time Stamp Interrupt Trigger */
#define ETH_PTP_FLAG_TSSTU        ((uint32_t)0x00000008)  /*!< Time Stamp Update */
#define ETH_PTP_FLAG_TSSTI        ((uint32_t)0x00000004)  /*!< Time Stamp Initialize */

#define ETH_PTP_FLAG_TSTTR        ((uint32_t)0x10000002)  /* Time stamp target time reached */
#define ETH_PTP_FLAG_TSSO         ((uint32_t)0x10000001)  /* Time stamp seconds overflow */

#define IS_ETH_PTP_GET_FLAG(FLAG) (((FLAG) == ETH_PTP_FLAG_TSARU) || \
                                    ((FLAG) == ETH_PTP_FLAG_TSITE) || \
                                    ((FLAG) == ETH_PTP_FLAG_TSSTU) || \
                                    ((FLAG) == ETH_PTP_FLAG_TSSTI) || \
                                    ((FLAG) == ETH_PTP_FLAG_TSTTR) || \
                                    ((FLAG) == ETH_PTP_FLAG_TSSO)) 

/** 
  * @brief  ETH PTP subsecond increment  
  */ 
#define IS_ETH_PTP_SUBSECOND_INCREMENT(SUBSECOND) ((SUBSECOND) <= 0xFF)

/**
  * @}
  */ 


/** @defgroup ETH_PTP_time_sign 
  * @{
  */ 
#define ETH_PTP_PositiveTime      ((uint32_t)0x00000000)  /*!< Positive time value */
#define ETH_PTP_NegativeTime      ((uint32_t)0x80000000)  /*!< Negative time value */
#define IS_ETH_PTP_TIME_SIGN(SIGN) (((SIGN) == ETH_PTP_PositiveTime) || \
                                    ((SIGN) == ETH_PTP_NegativeTime))

/** 
  * @brief  ETH PTP time stamp low update  
  */ 
#define IS_ETH_PTP_TIME_STAMP_UPDATE_SUBSECOND(SUBSECOND) ((SUBSECOND) <= 0x7FFFFFFF)

/** 
  * @brief  ETH PTP registers  
  */ 
#define ETH_PTPTSCR     ((uint32_t)0x00000700)  /*!< PTP TSCR register */
#define ETH_PTPSSIR     ((uint32_t)0x00000704)  /*!< PTP SSIR register */
#define ETH_PTPTSHR     ((uint32_t)0x00000708)  /*!< PTP TSHR register */
#define ETH_PTPTSLR     ((uint32_t)0x0000070C)  /*!< PTP TSLR register */
#define ETH_PTPTSHUR    ((uint32_t)0x00000710)  /*!< PTP TSHUR register */
#define ETH_PTPTSLUR    ((uint32_t)0x00000714)  /*!< PTP TSLUR register */
#define ETH_PTPTSAR     ((uint32_t)0x00000718)  /*!< PTP TSAR register */
#define ETH_PTPTTHR     ((uint32_t)0x0000071C)  /*!< PTP TTHR register */
#define ETH_PTPTTLR     ((uint32_t)0x00000720)  /* PTP TTLR register */

#define ETH_PTPTSSR     ((uint32_t)0x00000728)  /* PTP TSSR register */

#define IS_ETH_PTP_REGISTER(REG) (((REG) == ETH_PTPTSCR) || ((REG) == ETH_PTPSSIR) || \
                                   ((REG) == ETH_PTPTSHR) || ((REG) == ETH_PTPTSLR) || \
                                   ((REG) == ETH_PTPTSHUR) || ((REG) == ETH_PTPTSLUR) || \
                                   ((REG) == ETH_PTPTSAR) || ((REG) == ETH_PTPTTHR) || \
                                   ((REG) == ETH_PTPTTLR) || ((REG) == ETH_PTPTSSR)) 

/** 
  * @brief  ETHERNET PTP clock  
  */ 
#define ETH_PTP_OrdinaryClock               ((uint32_t)0x00000000)  /* Ordinary Clock */
#define ETH_PTP_BoundaryClock               ((uint32_t)0x00010000)  /* Boundary Clock */
#define ETH_PTP_EndToEndTransparentClock    ((uint32_t)0x00020000)  /* End To End Transparent Clock */
#define ETH_PTP_PeerToPeerTransparentClock  ((uint32_t)0x00030000)  /* Peer To Peer Transparent Clock */

#define IS_ETH_PTP_TYPE_CLOCK(CLOCK) (((CLOCK) == ETH_PTP_OrdinaryClock) || \
                          ((CLOCK) == ETH_PTP_BoundaryClock) || \
                          ((CLOCK) == ETH_PTP_EndToEndTransparentClock) || \
                                      ((CLOCK) == ETH_PTP_PeerToPeerTransparentClock))
/** 
  * @brief  ETHERNET snapshot
  */
#define ETH_PTP_SnapshotMasterMessage          ((uint32_t)0x00008000)  /* Time stamp snapshot for message relevant to master enable */
#define ETH_PTP_SnapshotEventMessage           ((uint32_t)0x00004000)  /* Time stamp snapshot for event message enable */
#define ETH_PTP_SnapshotIPV4Frames             ((uint32_t)0x00002000)  /* Time stamp snapshot for IPv4 frames enable */
#define ETH_PTP_SnapshotIPV6Frames             ((uint32_t)0x00001000)  /* Time stamp snapshot for IPv6 frames enable */
#define ETH_PTP_SnapshotPTPOverEthernetFrames  ((uint32_t)0x00000800)  /* Time stamp snapshot for PTP over ethernet frames enable */
#define ETH_PTP_SnapshotAllReceivedFrames      ((uint32_t)0x00000100)  /* Time stamp snapshot for all received frames enable */

#define IS_ETH_PTP_SNAPSHOT(SNAPSHOT) (((SNAPSHOT) == ETH_PTP_SnapshotMasterMessage) || \
                           ((SNAPSHOT) == ETH_PTP_SnapshotEventMessage) || \
                           ((SNAPSHOT) == ETH_PTP_SnapshotIPV4Frames) || \
                           ((SNAPSHOT) == ETH_PTP_SnapshotIPV6Frames) || \
                           ((SNAPSHOT) == ETH_PTP_SnapshotPTPOverEthernetFrames) || \
                           ((SNAPSHOT) == ETH_PTP_SnapshotAllReceivedFrames))

/** 
  * @brief  PTP  
  */ 
void ETH_EnablePTPTimeStampAddend(void);
void ETH_EnablePTPTimeStampInterruptTrigger(void);
void ETH_EnablePTPTimeStampUpdate(void);
void ETH_InitializePTPTimeStamp(void);
void ETH_PTPUpdateMethodConfig(uint32_t UpdateMethod);
void ETH_PTPTimeStampCmd(FunctionalState NewState);
FlagStatus ETH_GetPTPFlagStatus(uint32_t ETH_PTP_FLAG);
void ETH_SetPTPSubSecondIncrement(uint32_t SubSecondValue);
void ETH_SetPTPTimeStampUpdate(uint32_t Sign, uint32_t SecondValue, uint32_t SubSecondValue);
void ETH_SetPTPTimeStampAddend(uint32_t Value);
void ETH_SetPTPTargetTime(uint32_t HighValue, uint32_t LowValue);
uint32_t ETH_GetPTPRegister(uint32_t ETH_PTPReg);

typedef struct ptptime_s
{
  int32_t tv_sec;
  int32_t tv_nsec;
} ptptime_t;

void ethptp_start(uint32_t update_method);
void ethptp_get_time(ptptime_t *timestamp);
void ethptp_set_time(ptptime_t *timestamp);
void ethptp_adj_freq(int32_t adj_ppb);

#ifdef __cplusplus
}
#endif

#endif /* __ETH_PTP_H__ */
