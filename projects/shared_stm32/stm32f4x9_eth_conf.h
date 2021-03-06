/**
  ******************************************************************************
  * @file    stm32f4x9_eth_conf_template.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    31-July-2013
  * @brief   Configuration file for the STM32F4x9xx Ethernet driver.
  *          This file should be copied to the application folder and renamed to
  *          stm32f4x9_eth_conf.h    
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4x9_ETH_CONF_H
#define __STM32F4x9_ETH_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ll_drivers.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Uncomment the line below when using time stamping and/or IPv4 checksum offload */
#define USE_ENHANCED_DMA_DESCRIPTORS

/* Uncomment the line below if you want to use user defined Delay function
   (for precise timing), otherwise default _eth_delay_ function defined within
   the Ethernet driver is used (less precise timing) */
#ifdef __CMSIS_RTOS
#define USE_Delay
#endif

#ifdef USE_Delay
/* User can provide more timing precise _eth_delay_ function
   ex. use Systick with time base of 10 ms */
#include "cmsis_os2.h"
static inline void ETH_RTOS_Delay(uint32_t nCount)
{
  // Delay 10ms.
  osDelay(10);
}
#define _eth_delay_    ETH_RTOS_Delay
#else
/* Default _eth_delay_ function with less precise timing */
#define _eth_delay_    ETH_Delay
#endif

/* Uncomment the line below to allow custom configuration of the Ethernet driver buffers */    
//#define CUSTOM_DRIVER_BUFFERS_CONFIG   

#ifdef  CUSTOM_DRIVER_BUFFERS_CONFIG
/* Redefinition of the Ethernet driver buffers size and count */   
 #define ETH_RX_BUF_SIZE    ETH_MAX_PACKET_SIZE /* buffer size for receive */
 #define ETH_TX_BUF_SIZE    ETH_MAX_PACKET_SIZE /* buffer size for transmit */
 #define ETH_RXBUFNB        20                  /* 20 Rx buffers of size ETH_RX_BUF_SIZE */
 #define ETH_TXBUFNB        5                   /* 5  Tx buffers of size ETH_TX_BUF_SIZE */
#endif


/* PHY configuration section **************************************************/
#ifdef USE_Delay
/* PHY Reset delay */ 
#define PHY_RESET_DELAY    ((uint32_t)0x000000FF)
/* PHY Configuration delay */
#define PHY_CONFIG_DELAY   ((uint32_t)0x00000FFF)
/* Delay when writing to Ethernet registers*/
#define ETH_REG_WRITE_DELAY ((uint32_t)0x00000001)
#else
/* PHY Reset delay */ 
#define PHY_RESET_DELAY    ((uint32_t)0x000FFFFF)
/* PHY Configuration delay */ 
#define PHY_CONFIG_DELAY   ((uint32_t)0x00FFFFFF)
/* Delay when writing to Ethernet registers*/
#define ETH_REG_WRITE_DELAY ((uint32_t)0x0000FFFF)
#endif

/*******************  PHY Extended Registers section : ************************/

/* These values are relatives to LAN8742 PHY and change from PHY to another,
   so the user have to update this value depending on the used external PHY */   

#define PHY_ANAR               ((uint16_t)0x4)
#define PHY_ANAR_NP            ((uint16_t)0x8000)
#define PHY_ANAR_RF            ((uint16_t)0x2000)
#define PHY_ANAR_PAUSE1        ((uint16_t)0x0800)
#define PHY_ANAR_PAUSE0        ((uint16_t)0x0400)
#define PHY_ANAR_100BT4        ((uint16_t)0x0200)
#define PHY_ANAR_100BTX_FD     ((uint16_t)0x0100)
#define PHY_ANAR_100BTX        ((uint16_t)0x0080)
#define PHY_ANAR_10BT_FD       ((uint16_t)0x0040)
#define PHY_ANAR_10BT          ((uint16_t)0x0020)
#define PHY_ANAR_SELECTOR4     ((uint16_t)0x0010)
#define PHY_ANAR_SELECTOR3     ((uint16_t)0x0008)
#define PHY_ANAR_SELECTOR2     ((uint16_t)0x0004)
#define PHY_ANAR_SELECTOR1     ((uint16_t)0x0002)
#define PHY_ANAR_SELECTOR0     ((uint16_t)0x0001)

#define PHY_SR                 ((uint16_t)0x10)
#define PHY_SPEED_STATUS       ((uint16_t)0x0002)
#define PHY_DUPLEX_STATUS      ((uint16_t)0x0004)

   /* Note : Common PHY registers are defined in stm32f4x9_eth.h file */

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4x9_ETH_CONF_H */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
