#include <string.h>
#include <stdlib.h>
#include "hal_system.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/ethip6.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "delay.h"
#include "outputf.h"
#include "console.h"
#include "syslog.h"
#include "hardtime.h"
#include "network.h"
#include "ethptp.h"
#include "ethernetif.h"

// Ethernet event flag values.
#define ETHERNETIF_EVENT_RECEIVE         0x0001u
#define ETHERNETIF_EVENT_TRANSMIT        0x0002u
#define ETHERNETIF_EVENT_TIMER           0x0004u

#if LWIP_PTPD
// PTP frame parsing helper macros.
#define ENET_PTP1588_EVENT_PORT 319U
#define ENET_PTP1588_GENERAL_PORT 320U
#define ENET_PTP1588_ETHL2_MSGTYPE 3U
#define ENET_PTP1588_IPVERSION_OFFSET 0x0EU
#define ENET_PTP1588_ETHL2_MSGTYPE_OFFSET 0x0EU
#define ENET_PTP1588_ETHL2_PACKETTYPE_OFFSET 0x0CU
#define ENET_PTP1588_IPV4_UDP_PROTOCOL_OFFSET 0x17U
#define ENET_PTP1588_IPV4_UDP_PORT_OFFSET 0x24U
#define ENET_PTP1588_IPV6_UDP_PROTOCOL_OFFSET 0x14U
#define ENET_PTP1588_IPV6_UDP_PORT_OFFSET 0x38U
#define ENET_IPV4 0x0800U
#define ENET_IPV6 0x86ddU
#define ENET_IPV4VERSION 0x0004U
#define ENET_IPV6VERSION 0x0006U
#define ENET_UDPVERSION 0x0011U
#define ENET_ETHERNETL2 0x88F7U
#define ENET_8021QVLAN 0x8100U
#define ENET_FRAME_VLAN_TAGLEN 4U
#endif

// Macro to define section. On STM32F7 architectures we must be certain
// the Ethernet buffers are placed into memory suitable for DMA.
#if defined(STM32F7)
#if (defined (__GNUC__) || defined (__ARMCC_VERSION)) && !defined (__CC_ARM)
  #ifndef __SECTION_NAME
    #define __SECTION_NAME(name)    __attribute__((section(name)))
  #endif
#else
  #ifndef __SECTION_NAME
    #define __SECTION_NAME(name)    __attribute__((section(name)))
  #endif
#endif
#else
    #define __SECTION_NAME(name)
#endif

// Ethernet receive DMA descriptors.
__ALIGN_BEGIN ETH_DMADescTypeDef dma_rx_descriptor_table[ETH_RXBUFNB] __SECTION_NAME("dtcm") __ALIGN_END;

// Ethernet transmit DMA descriptors.
__ALIGN_BEGIN ETH_DMADescTypeDef dma_tx_descriptor_table[ETH_TXBUFNB] __SECTION_NAME("dtcm") __ALIGN_END;

// Ethernet receive buffers.
__ALIGN_BEGIN uint8_t dma_rx_buffer[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __SECTION_NAME("dtcm") __ALIGN_END;

// Ethernet transmit buffers.
__ALIGN_BEGIN uint8_t dma_tx_buffer[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __SECTION_NAME("dtcm") __ALIGN_END;

// Ethernet mutex identifier.
static osMutexId_t ethernetif_mutex_id = NULL;

// Ethernet timer identifier.
static osTimerId_t ethernetif_timer_id = NULL;

// Ethernet thread identifier.
static osThreadId_t ethernetif_thread_id = NULL;

// Ethernet event flags to signal Ethernet TX/RX events.
static osEventFlagsId_t ethernetif_event_id = NULL;

// Ethernet link status flag. This is true when the link is up.
static bool ethernetif_link_status = false;

// Ethernet link counts.
static uint32_t ethernetif_recv_count = 0u;
static uint32_t ethernetif_recv_bytes = 0u;
static uint32_t ethernetif_send_count = 0u;
static uint32_t ethernetif_send_bytes = 0u;

// Ethernet interface handle.
static ETH_HandleTypeDef ethernetif_handle;

#if LWIP_PTPD
// Ethernet TX entry for tracking timestamps.
typedef struct ethernet_tx_entry_s
{
  int32_t id;
  __IO ETH_DMADescTypeDef *tx_desc;
} ethernet_tx_entry_t;
static uint32_t ethernet_tx_head = 0;
static uint32_t ethernet_tx_tail = 0;
static ethernet_tx_entry_t ethernet_tx_entries[ETH_TXBUFNB];
#endif

#if LWIP_PTPD
// Conversion from hardware to PTP format.
static uint32_t subsecond_to_nanosecond(uint32_t subsecond_value)
{
  uint64_t val = subsecond_value * 1000000000ll;
  val >>= 31;
  return val;
}
#endif

//
// HAL Ethernet callbacks.
//

// This function handles Ethernet global interrupt.
void ETH_IRQHandler(void)
{
  // Call the HAL Ethernet interrupt handler.
  HAL_ETH_IRQHandler(&ethernetif_handle);
}

// Ethernet Rx transfer complete callback from the HAL.
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *eth_handle)
{
  UNUSED(eth_handle);

  // Notify the Ethernet thread of the incoming packets.
  osEventFlagsSet(ethernetif_event_id, ETHERNETIF_EVENT_RECEIVE);
}

// Ethernet Tx transfer complete callback from the HAL.
void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *eth_handle)
{
  UNUSED(eth_handle);

  // Notify the Ethernet thread of the outgoing packet complete.
  osEventFlagsSet(ethernetif_event_id, ETHERNETIF_EVENT_TRANSMIT);
}

//
// LWIP Ethernet driver.
//

#if LWIP_PTPD
// Returns true if the packet is a PTP frame.  This is to avoid waiting for
// the timestamp to become available when transmitting non-PTP packets.
static bool is_ptp1588_frame(uint8_t *data)
{
  uint8_t *buffer = data;
  uint16_t ptp_type;
  bool is_ptp_frame = false;

  // Check for VLAN frame.
  if (*(uint16_t *)(buffer + ENET_PTP1588_ETHL2_PACKETTYPE_OFFSET) == lwip_htons(ENET_8021QVLAN))
  {
    buffer += ENET_FRAME_VLAN_TAGLEN;
  }

  ptp_type = *((uint16_t *)(buffer + ENET_PTP1588_ETHL2_PACKETTYPE_OFFSET));
  switch (lwip_htons(ptp_type))
  {
    // Ethernet layer 2.
    case ENET_ETHERNETL2:
      if (*(uint8_t *)(buffer + ENET_PTP1588_ETHL2_MSGTYPE_OFFSET) <= ENET_PTP1588_ETHL2_MSGTYPE)
      {
        // This is a PTP frame.
        is_ptp_frame = true;
      }
      break;
    // IPV4.
    case ENET_IPV4:
      if ((*(uint8_t *)(buffer + ENET_PTP1588_IPVERSION_OFFSET) >> 4) == ENET_IPV4VERSION)
      {
        if (((*(uint16_t *)(buffer + ENET_PTP1588_IPV4_UDP_PORT_OFFSET)) == lwip_htons(ENET_PTP1588_EVENT_PORT)) &&
            (*(uint8_t *)(buffer + ENET_PTP1588_IPV4_UDP_PROTOCOL_OFFSET) == ENET_UDPVERSION))
        {
          // This is a PTP frame.
          is_ptp_frame = true;
        }
      }
      break;
    // IPV6.
    case ENET_IPV6:
      if ((*(uint8_t *)(buffer + ENET_PTP1588_IPVERSION_OFFSET) >> 4) == ENET_IPV6VERSION)
      {
        if (((*(uint16_t *)(buffer + ENET_PTP1588_IPV6_UDP_PORT_OFFSET)) == lwip_htons(ENET_PTP1588_EVENT_PORT)) &&
            (*(uint8_t *)(buffer + ENET_PTP1588_IPV6_UDP_PROTOCOL_OFFSET) == ENET_UDPVERSION))
        {
          // This is a PTP frame.
          is_ptp_frame = true;
        }
      }
      break;
    default:
      break;
  }

  return is_ptp_frame;
}
#endif

// This function should does the actual transmission of the packet. The packet
// (IP packet including MAC addresses and type) is contained in the pbuf that is
// passed to the function. This pbuf might be chained. Returns ERR_OK if the
// packet could be sent or an err_t value if the packet couldn't be sent.
//
// Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
// strange results. You might consider waiting for space in the DMA queue to
// become availale since the stack doesn't retry to send a packet dropped
// because of memory failure (except for the TCP timers).
static err_t ethernetif_linkoutput(struct netif *netif, struct pbuf *p)
{
  err_t errval;
  struct pbuf *q;
  uint8_t *buffer;
  __IO ETH_DMADescTypeDef *dma_tx_desc;
  uint32_t framelength = 0;
  uint32_t bufferoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t payloadoffset = 0;
  HAL_StatusTypeDef hal_status;
#if LWIP_PTPD
  bool is_ptp;
#endif

#if ETH_PAD_SIZE
  // Drop the padding word. Without the padding adjustment addresses in the IP header will
  // not be aligned on a 32-bit boundary, so setting this to 2 can speed up 32-bit-platforms.
  pbuf_header(p, -ETH_PAD_SIZE);
#endif

  // Point to the first transmit DMA descriptor.
  dma_tx_desc = ethernetif_handle.TxDesc;

  // Point to the transmit buffer associated with the transmit descriptor.
  buffer = (uint8_t *) (dma_tx_desc->Buffer1Addr);

#if LWIP_PTPD
  // Clear the timestamp information.
  p->time_sec = 0;
  p->time_nsec = 0;

  // Does this look like a PTP IEEE 1588 frame?
  is_ptp = is_ptp1588_frame(buffer);
#endif

  // Copy frame from pbufs to driver buffers.
  for (q = p; q != NULL; q = q->next)
  {
    // Is this buffer available? If not, goto error.
    if ((dma_tx_desc->Status & ETH_DMATXDESC_OWN) != (uint32_t) RESET)
    {
      errval = ERR_USE;
      goto error;
    }

    // Get bytes in current lwIP buffer.
    byteslefttocopy = q->len;
    payloadoffset = 0;

    // Check if the length of data to copy is bigger than transmit buffer size.
    while ((byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE)
    {
      // Copy data to the transmit buffer.
      memcpy((uint8_t*) ((uint8_t*) buffer + bufferoffset), (uint8_t*) ((uint8_t*) q->payload + payloadoffset), (ETH_TX_BUF_SIZE - bufferoffset));

      // Point to next descriptor.
      dma_tx_desc = (ETH_DMADescTypeDef *)(dma_tx_desc->Buffer2NextDescAddr);

      // Point to the transmit buffer associated with the transmit descriptor.
      buffer = (uint8_t *) (dma_tx_desc->Buffer1Addr);

      // Verify the buffer is available.
      if ((dma_tx_desc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
      {
        errval = ERR_USE;
        goto error;
      }

      byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
      payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
      framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
      bufferoffset = 0;
    }

    // Copy the remaining bytes to the transmit buffer.
    memcpy((uint8_t*) ((uint8_t*) buffer + bufferoffset), (uint8_t*) ((uint8_t*) q->payload + payloadoffset), byteslefttocopy);
    bufferoffset = bufferoffset + byteslefttocopy;
    framelength = framelength + byteslefttocopy;
  }

#if 0
  {
    // Get the timestamp.
    int64_t timestamp = hardtime_get();

    // Break into two pieces -- seconds and nanoseconds.
    int32_t top = timestamp / 1000000000;
    int32_t bot = abs(((int32_t) (timestamp % 1000000000)) / 1000);

    // Print the received buffer.
    outputf("Transmitted %d.%06d:\n", top, bot);
    for (uint32_t i = 0; i < framelength; ++i)
    {
      outputf("%02x ", (uint32_t) buffer[i]);
      if ((i & 0x07) == 0x07) outputf(" ");
      if ((i & 0x0f) == 0x0f) outputf("\n");
    }
    outputf("\n\n");
  }
#endif

  // Clear the transmit event flag before sending the frame.
  osEventFlagsClear(ethernetif_event_id, ETHERNETIF_EVENT_TRANSMIT);

  // Lock the Ethernet mutex to prevent reentrant calls into HAL Ethernet code.
  osMutexAcquire(ethernetif_mutex_id, osWaitForever);

  // Transmit the frame.
  hal_status = HAL_ETH_TransmitFrame_IT(&ethernetif_handle, framelength);

  // Increment the interface send count and bytes.
  if (hal_status == HAL_OK)
  {
    ethernetif_send_count += 1;
    ethernetif_send_bytes += framelength;
  }

#if LWIP_PTPD
  // Keep track of the DMA TX descriptors used for PTP frames.
  if ((hal_status == HAL_OK) && is_ptp)
  {
    // Unique id for each packet sent. Range 1 to 10000.
    static int32_t unique_id = 1;

    // Save a unique id in the nanosecond field. This is a convienent place
    // to save the unique id in the packet buffer we can later use to retrieve
    // the timestamp information after the is sent.
    p->time_sec = 0;
    p->time_nsec = unique_id;

    // Update the unique id keeping it non-zero.
    unique_id = (unique_id + 1) > 10000 ? 1 : unique_id + 1;

    // Fill in the next entry to use.
    ethernet_tx_entries[ethernet_tx_head].id = p->time_nsec;
    ethernet_tx_entries[ethernet_tx_head].tx_desc = dma_tx_desc;

    // Increment the head.
    ethernet_tx_head = (ethernet_tx_head + 1) == ETH_TXBUFNB ? 0 : ethernet_tx_head + 1;

    // Increment the tail if we have wrapped.
    if (ethernet_tx_head == ethernet_tx_tail)
      ethernet_tx_tail = (ethernet_tx_tail + 1) == ETH_TXBUFNB ? 0 : ethernet_tx_tail + 1;
  }
#endif

  // Release the Ethernet mutex.
  osMutexRelease(ethernetif_mutex_id);

  // Prepare transmit descriptors to give to DMA.
  if (hal_status != HAL_OK)
  {
    // Report an error.
    syslog_printf(SYSLOG_ERROR, "ETHERNETIF: error sending packet");
  }

#if ETH_PAD_SIZE
  // Reclaim the padding word.
  pbuf_header(p, ETH_PAD_SIZE);
#endif

  errval = ERR_OK;

error:

  // When Transmit Underflow flag is set, clear it and issue a Transmit Poll Demand to resume transmission.
  if ((ethernetif_handle.Instance->DMASR & ETH_DMASR_TUS) != (uint32_t) RESET)
  {
    // Clear TUS ETHERNET DMA flag.
    ethernetif_handle.Instance->DMASR = ETH_DMASR_TUS;

    // Resume DMA transmission.
    ethernetif_handle.Instance->DMATPDR = 0;
  }

#if ETH_PAD_SIZE
  // Reclaim the padding word.
  pbuf_header(p, ETH_PAD_SIZE);
#endif

  return errval;
}

// Handles the receiving of the incoming packets by allocating a pbuf
// and transfering the bytes of the incoming packet from the interface
// into the pbuf.  Returns a pbuf filled with the received packet
// (including MAC header) NULL on memory error.
static struct pbuf *ethernetif_linkinput(struct netif *netif)
{
  struct pbuf *p = NULL;
  struct pbuf *q = NULL;
  uint16_t len = 0;
  uint8_t *buffer;
  __IO ETH_DMADescTypeDef *dma_rx_desc;
  uint32_t bufferoffset = 0;
  uint32_t payloadoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t i = 0;
  HAL_StatusTypeDef hal_status;

  // Lock the Ethernet mutex to prevent reentrant calls into HAL Ethernet code.
  osMutexAcquire(ethernetif_mutex_id, osWaitForever);

  // Get received frame.
  hal_status = HAL_ETH_GetReceivedFrame_IT(&ethernetif_handle);

  // Increment the interface receive count and bytes.
  if (hal_status == HAL_OK)
  {
    ethernetif_recv_count += 1;
    ethernetif_recv_bytes += ethernetif_handle.RxFrameInfos.length;
  }

  // Release the Ethernet mutex.
  osMutexRelease(ethernetif_mutex_id);

  // Get received frame.
  if (hal_status == HAL_OK)
  {
    // Obtain the size of the packet and put it into the "len" variable.
    len = ethernetif_handle.RxFrameInfos.length;
    buffer = (uint8_t *) ethernetif_handle.RxFrameInfos.buffer;

    if (len > 0)
    {
#if ETH_PAD_SIZE
      // Allow room for Ethernet padding.
      len += ETH_PAD_SIZE;
#endif

      // We allocate a pbuf chain of pbufs from the Lwip buffer pool.
      p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    }

    if (p != NULL)
    {
#if 0
      {
        // Get the timestamp.
        int64_t timestamp = hardtime_get();

        // Break into two pieces -- seconds and nanoseconds.
        int32_t top = timestamp / 1000000000;
        int32_t bot = abs(((int32_t) (timestamp % 1000000000)) / 1000);

        // Print the received buffer.
        outputf("Received %d.%06d:\n", top, bot);
        for (uint32_t i = 0; i < ethernetif_handle.RxFrameInfos.length; ++i)
        {
          outputf("%02x ", (uint32_t) ((uint8_t *)ethernetif_handle.RxFrameInfos.buffer)[i]);
          if ((i & 0x07) == 0x07) outputf(" ");
          if ((i & 0x0f) == 0x0f) outputf("\n");
        }
        outputf("\n\n");
      }
#endif

#if ETH_PAD_SIZE
      // Drop the padding word.
      pbuf_header(p, -ETH_PAD_SIZE);
#endif

      dma_rx_desc = ethernetif_handle.RxFrameInfos.FSRxDesc;
      bufferoffset = 0;
      for (q = p; q != NULL; q = q->next)
      {
        byteslefttocopy = q->len;
        payloadoffset = 0;

        // Check if the length of bytes to copy in current pbuf is bigger than Rx buffer size.
        while ((byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE)
        {
          // Copy data to pbuf.
          memcpy((uint8_t*) ((uint8_t*) q->payload + payloadoffset), (uint8_t*) ((uint8_t*) buffer + bufferoffset), (ETH_RX_BUF_SIZE - bufferoffset));

          // Point to next descriptor.
          dma_rx_desc = (ETH_DMADescTypeDef *) (dma_rx_desc->Buffer2NextDescAddr);
          buffer = (uint8_t *) (dma_rx_desc->Buffer1Addr);

          byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
          payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
          bufferoffset = 0;
        }

        // Copy remaining data in pbuf.
        memcpy((uint8_t*) ((uint8_t*) q->payload + payloadoffset), (uint8_t*) ((uint8_t*) buffer + bufferoffset), byteslefttocopy);
        bufferoffset = bufferoffset + byteslefttocopy;
      }

#if LWIP_PTPD
      // Copy the frame timestamp.
      p->time_sec = ethernetif_handle.RxFrameInfos.FSRxDesc->TimeStampHigh;
      p->time_nsec = subsecond_to_nanosecond(ethernetif_handle.RxFrameInfos.FSRxDesc->TimeStampLow);
#endif

#if ETH_PAD_SIZE
      // Reclaim the padding word.
      pbuf_header(p, ETH_PAD_SIZE);
#endif
    }

    // Release descriptors to DMA .
    // Point to first descriptor.
    dma_rx_desc = ethernetif_handle.RxFrameInfos.FSRxDesc;

    // Set Own bit in Rx descriptors: gives the buffers back to DMA.
    for (i = 0; i < ethernetif_handle.RxFrameInfos.SegCount; ++i)
    {
      dma_rx_desc->Status |= ETH_DMARXDESC_OWN;
      dma_rx_desc = (ETH_DMADescTypeDef *) (dma_rx_desc->Buffer2NextDescAddr);
    }

    // Clear the segment count.
    ethernetif_handle.RxFrameInfos.SegCount = 0;

    // When Rx Buffer unavailable flag is set: clear it and resume reception.
    if ((ethernetif_handle.Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t) RESET)
    {
      // Clear RBUS ETHERNET DMA flag.
      ethernetif_handle.Instance->DMASR = ETH_DMASR_RBUS;

      // Resume DMA reception.
      ethernetif_handle.Instance->DMARPDR = 0;
    }
  }

  return p;
}

// Configure the Ethernet MAC and DMA.
static void ethernetif_link_config(struct netif *netif)
{
  // Lock the Ethernet mutex to prevent reentrant calls into HAL Ethernet code.
  osMutexAcquire(ethernetif_mutex_id, osWaitForever);

  // Initializes the Ethernet MAC and DMA according to default parameters.
  memset(&ethernetif_handle, 0, sizeof(ethernetif_handle));
  ethernetif_handle.Instance = ETH;
  ethernetif_handle.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
  ethernetif_handle.Init.PhyAddress = LAN8742A_PHY_ADDRESS;
  ethernetif_handle.Init.MACAddr = &netif->hwaddr[0];
  ethernetif_handle.Init.RxMode = ETH_RXINTERRUPT_MODE;
  ethernetif_handle.Init.TxMode = ETH_TXINTERRUPT_MODE;
  ethernetif_handle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
  ethernetif_handle.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;
  HAL_ETH_Init(&ethernetif_handle);

  // Initialize Tx and Rx Descriptors list: Chain Mode.
  HAL_ETH_DMATxDescListInit(&ethernetif_handle, dma_tx_descriptor_table, &dma_tx_buffer[0][0], ETH_TXBUFNB);
  HAL_ETH_DMARxDescListInit(&ethernetif_handle, dma_rx_descriptor_table, &dma_rx_buffer[0][0], ETH_RXBUFNB);

  // Initialize custom MAC parameters.
  // NOTE: the MulticastFramesFilter is set to none for support of MDNS packets.
  ETH_MACInitTypeDef mac_init;
  memset(&mac_init, 0, sizeof(mac_init));
  mac_init.Watchdog = ETH_WATCHDOG_ENABLE;
  mac_init.Jabber = ETH_JABBER_ENABLE;
  mac_init.InterFrameGap = ETH_INTERFRAMEGAP_96BIT;
  mac_init.CarrierSense = ETH_CARRIERSENCE_ENABLE;
  mac_init.ReceiveOwn = ETH_RECEIVEOWN_ENABLE;
  mac_init.LoopbackMode = ETH_LOOPBACKMODE_DISABLE;
  mac_init.ChecksumOffload = ETH_CHECKSUMOFFLAOD_ENABLE;
  mac_init.RetryTransmission = ETH_RETRYTRANSMISSION_DISABLE;
  mac_init.AutomaticPadCRCStrip = ETH_AUTOMATICPADCRCSTRIP_DISABLE;
  mac_init.BackOffLimit = ETH_BACKOFFLIMIT_10;
  mac_init.DeferralCheck = ETH_DEFFERRALCHECK_DISABLE;
  mac_init.ReceiveAll = ETH_RECEIVEAll_DISABLE;
  mac_init.SourceAddrFilter = ETH_SOURCEADDRFILTER_DISABLE;
  mac_init.PassControlFrames = ETH_PASSCONTROLFRAMES_BLOCKALL;
  mac_init.BroadcastFramesReception = ETH_BROADCASTFRAMESRECEPTION_ENABLE;
  mac_init.DestinationAddrFilter = ETH_DESTINATIONADDRFILTER_NORMAL;
  mac_init.PromiscuousMode = ETH_PROMISCUOUS_MODE_DISABLE;
  mac_init.MulticastFramesFilter = ETH_MULTICASTFRAMESFILTER_NONE;
  mac_init.UnicastFramesFilter = ETH_UNICASTFRAMESFILTER_PERFECT;
  mac_init.HashTableHigh = 0x0U;
  mac_init.HashTableLow = 0x0U;
  mac_init.PauseTime = 0x0U;
  mac_init.ZeroQuantaPause = ETH_ZEROQUANTAPAUSE_DISABLE;
  mac_init.PauseLowThreshold = ETH_PAUSELOWTHRESHOLD_MINUS4;
  mac_init.UnicastPauseFrameDetect = ETH_UNICASTPAUSEFRAMEDETECT_DISABLE;
  mac_init.ReceiveFlowControl = ETH_RECEIVEFLOWCONTROL_DISABLE;
  mac_init.TransmitFlowControl = ETH_TRANSMITFLOWCONTROL_DISABLE;
  mac_init.VLANTagComparison = ETH_VLANTAGCOMPARISON_16BIT;
  mac_init.VLANTagIdentifier = 0x0U;
  HAL_ETH_ConfigMAC(&ethernetif_handle, &mac_init);

  // Initialize custom DMA parameters.
  ETH_DMAInitTypeDef dma_init;
  memset(&dma_init, 0, sizeof(dma_init));
  dma_init.DropTCPIPChecksumErrorFrame = ETH_DROPTCPIPCHECKSUMERRORFRAME_ENABLE;
  dma_init.ReceiveStoreForward = ETH_RECEIVESTOREFORWARD_ENABLE;
  dma_init.FlushReceivedFrame = ETH_FLUSHRECEIVEDFRAME_ENABLE;
  dma_init.TransmitStoreForward = ETH_TRANSMITSTOREFORWARD_ENABLE;  
  dma_init.TransmitThresholdControl = ETH_TRANSMITTHRESHOLDCONTROL_64BYTES;
  dma_init.ForwardErrorFrames = ETH_FORWARDERRORFRAMES_DISABLE;
  dma_init.ForwardUndersizedGoodFrames = ETH_FORWARDUNDERSIZEDGOODFRAMES_DISABLE;
  dma_init.ReceiveThresholdControl = ETH_RECEIVEDTHRESHOLDCONTROL_64BYTES;
  dma_init.SecondFrameOperate = ETH_SECONDFRAMEOPERARTE_ENABLE;
  dma_init.AddressAlignedBeats = ETH_ADDRESSALIGNEDBEATS_ENABLE;
  dma_init.FixedBurst = ETH_FIXEDBURST_ENABLE;
  dma_init.RxDMABurstLength = ETH_RXDMABURSTLENGTH_32BEAT;
  dma_init.TxDMABurstLength = ETH_TXDMABURSTLENGTH_32BEAT;
  dma_init.EnhancedDescriptorFormat = ETH_DMAENHANCEDDESCRIPTOR_ENABLE;
  dma_init.DescriptorSkipLength = 0x0U;
  dma_init.DMAArbitration = ETH_DMAARBITRATION_ROUNDROBIN_RXTX_1_1;
  HAL_ETH_ConfigDMA(&ethernetif_handle, &dma_init);

  // Enable interrupt on change of link status.
  uint32_t regvalue = 0;
  HAL_ETH_ReadPHYRegister(&ethernetif_handle, PHY_ISFR, &regvalue);
  regvalue |= PHY_ISFR_INT4;
  HAL_ETH_WritePHYRegister(&ethernetif_handle, PHY_ISFR , regvalue);

  // Enable PTP time stamping.
  ethptp_start(ETH_PTP_FineUpdate);

  // Release the Ethernet mutex.
  osMutexRelease(ethernetif_mutex_id);
}

// Check if the link has gone up or down.
static void ethernetif_link_check(struct netif *netif)
{
  uint32_t phyreg = 0u;

  // Lock the Ethernet mutex to prevent reentrant calls into HAL Ethernet code.
  osMutexAcquire(ethernetif_mutex_id, osWaitForever);

  // Read the PHY register.
  HAL_ETH_ReadPHYRegister(&ethernetif_handle, PHY_BSR, &phyreg);

  // Release the Ethernet mutex.
  osMutexRelease(ethernetif_mutex_id);

  // Get the link established state.
  bool link_status = (phyreg & PHY_LINKED_STATUS) == PHY_LINKED_STATUS ? true : false;

  // Is the link going up or down?
  if (link_status && !ethernetif_link_status)
  {
    // Lock the Ethernet mutex to prevent reentrant calls into HAL Ethernet code.
    osMutexAcquire(ethernetif_mutex_id, osWaitForever);

    // Configure the Ethernet MAC and DMA.
    HAL_ETH_Config(&ethernetif_handle);

    // Enable MAC and DMA transmission and reception.
    HAL_ETH_Start(&ethernetif_handle);

    // The link is now up.
    ethernetif_link_status = true;
    ethernetif_handle.LinkStatus = 1u;

    // Release the Ethernet mutex.
    osMutexRelease(ethernetif_mutex_id);

    // Lock the lwIP core mutex.
    LOCK_TCPIP_CORE();

    // Notify the network interface link is up.
    netif_set_up(netif);
    netif_set_link_up(netif);

    // Unlock the lwIP core mutex.
    UNLOCK_TCPIP_CORE();
  }
  else if (!link_status && ethernetif_link_status)
  {
    // Lock the Ethernet mutex to prevent reentrant calls into HAL Ethernet code.
    osMutexAcquire(ethernetif_mutex_id, osWaitForever);

    // The link is now down.
    ethernetif_link_status = false;
    ethernetif_handle.LinkStatus = 0u;

    // Stop Ethernet MAC and DMA transmission and reception.
    HAL_ETH_Stop(&ethernetif_handle);

    // Release the Ethernet mutex.
    osMutexRelease(ethernetif_mutex_id);

    // Lock the lwIP core mutex.
    LOCK_TCPIP_CORE();

    // Notify the network interface link is down.
    netif_set_down(netif);
    netif_set_link_down(netif);

    // Unlock the lwIP core mutex.
    UNLOCK_TCPIP_CORE();
  }
}

// This thread handles the actual reception of packets from the 
// Ethernet interface. It uses the function ethernetif_linkinput() that
// should handle the actual reception of bytes from the network
// interface. Then the type of the received packet is determined and
// the appropriate input function is called.
static void ethernetif_thread(void *argument )
{
  // Dereference the pointer to the network interface.
  struct netif *netif = (struct netif *) argument;

  // Initial configuration of the Ethernet MAC and DMA.
  ethernetif_link_config(netif);

  // Start the periodic ethernet timer.
  if (ethernetif_timer_id) osTimerStart(ethernetif_timer_id, tick_from_milliseconds(1000));

  // Note that this thread must be a higher priority than the LWIP thread so that it
  // can preempt the LWIP thread when new data comes in. This is important for TCP processing.
  osThreadSetPriority(osThreadGetId(), osPriorityHigh1);

  // Loop forever.
  for(;;)
  {
    // Get the next low level input.
    struct pbuf *p = ethernetif_linkinput(netif);

    // Should we wait for an event?
    while (p == NULL)
    {
      // Set flags to wait for a receive or timer event.
      uint32_t flags = ETHERNETIF_EVENT_RECEIVE | ETHERNETIF_EVENT_TIMER;

      // Wait for an Ethernet receiver or timer event.
      flags = osEventFlagsWait(ethernetif_event_id, flags, osFlagsWaitAny, osWaitForever);

      // Do the flags indicate a receive event?
      if ((flags & ETHERNETIF_EVENT_RECEIVE) == ETHERNETIF_EVENT_RECEIVE)
      {
        // Get the next low level input.
        p = ethernetif_linkinput(netif);
      }

      // Do the flags indicate a timer event?
      if ((flags & ETHERNETIF_EVENT_TIMER) == ETHERNETIF_EVENT_TIMER)
      {
        // Test if the ethernet interface went up or down.
        ethernetif_link_check(netif);
      }
    }

    // Call into the interface input handler.
    if (netif->input(p, netif) != ERR_OK)
    {
      // Free the buffer here if there was an error.
      pbuf_free(p);
    }
  }
}

// Ethernet timer handler.
static void ethernetif_timer(void *arg)
{
  // Notify the timer event to be handled in the Ethernet thread.
  osEventFlagsSet(ethernetif_event_id, ETHERNETIF_EVENT_TIMER);
}

// Front end to etharp_output function to prevent sending when interface is down.
static err_t ethernetif_etharp_output(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{
  // Return error if interface is down.
  if (!netif_is_link_up(netif)) return ERR_RTE;

  // Pass along call.
  return etharp_output(netif, q, ipaddr);
}

#if LWIP_IPV6
// Front end to ethip6_output function to prevent sending when interface is down.
static err_t ethernetif_ethip6_output(struct netif *netif, struct pbuf *q, const ip6_addr_t *ip6addr)
{
  // Return error if interface is down.
  if (!netif_is_link_up(netif)) return ERR_RTE;

  // Pass along call.
  return ethip6_output(netif, q, ipaddr);
}
#endif

// This function should be passed as a parameter to netif_add() to setup
// network interface. Returns ERR_OK if initialization successful, ERR_MEM
// if private data couldn't be allocated or any other err_t on error.
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  // Initialize network hostname.
  netif->hostname = network_get_hostname();
#endif

  // Set abreviated interface description.
  netif->name[0] = 's';
  netif->name[1] = 't';

  // Set system configured hardware MAC address.
  hwaddr_t hwaddr = network_config_hwaddr();
  netif->hwaddr_len = ETH_HWADDR_LEN;
  netif->hwaddr[0] = hwaddr.hwaddr[0];
  netif->hwaddr[1] = hwaddr.hwaddr[1];
  netif->hwaddr[2] = hwaddr.hwaddr[2];
  netif->hwaddr[3] = hwaddr.hwaddr[3];
  netif->hwaddr[4] = hwaddr.hwaddr[4];
  netif->hwaddr[5] = hwaddr.hwaddr[5];

  // Set the maximum transfer unit.
  netif->mtu = 1500;

  // Accept broadcast address and ARP traffic.
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

#ifdef LWIP_IGMP
  // Accept multicast traffic.
  netif->flags |= NETIF_FLAG_IGMP;
#endif

  // Set callbacks for sending packets.
#if LWIP_IPV4
  netif->output = ethernetif_etharp_output;
#endif
#if LWIP_IPV6
  netif->output_ip6 = ethernetif_ethip6_output;
#endif
  netif->linkoutput = ethernetif_linkoutput;

  // Static event and thread control blocks.
  static uint32_t ethernet_event_cb[osRtxEventFlagsCbSize/4U] __attribute__((section(".bss.os.evflags.cb")));
  static uint32_t ethernet_timer_cb[osRtxTimerCbSize/4U] __attribute__((section(".bss.os.timer.cb")));
  static uint32_t ethernet_mutex_cb[osRtxMutexCbSize/4U] __attribute__((section(".bss.os.mutex.cb")));
  static uint32_t ethernet_thread_cb[osRtxThreadCbSize/4U] __attribute__((section(".bss.os.thread.cb")));

  // Ethernet mutex attributes. Not the mutex is not recursive.
  osMutexAttr_t ethernet_mutex_attrs =
  {
    .name = "ethernetif",
    .attr_bits = 0U, 
    .cb_mem = ethernet_mutex_cb,
    .cb_size = sizeof(ethernet_mutex_cb)
  };

  // Ethernet timer attributes.
  osTimerAttr_t ethernet_timer_attrs =
  {
    .name = "ethernetif",
    .attr_bits = 0U,
    .cb_mem = ethernet_timer_cb,
    .cb_size = sizeof(ethernet_timer_cb)
  };

  // Ethernet event attributes.
  osEventFlagsAttr_t ethernet_event_attrs =
  {
    .name = "ethernetif",
    .attr_bits = 0U,
    .cb_mem = ethernet_event_cb,
    .cb_size = sizeof(ethernet_event_cb)
  };

  // Ethernet thread attributes.
  osThreadAttr_t ethernet_thread_attrs =
  {
    .name = "ethernetif",
    .attr_bits = 0U,
    .cb_mem = ethernet_thread_cb,
    .cb_size = sizeof(ethernet_thread_cb),
    .stack_size = 4096,
    .priority = osPriorityNormal
  };

  // Create the Ethernet mutex.
  ethernetif_mutex_id = osMutexNew(&ethernet_mutex_attrs);
  if (ethernetif_mutex_id == NULL)
  {
    syslog_printf(SYSLOG_ERROR, "ETHERNETIF: cannot create mutex");
  }

  // Create the Ethernet timer.
  ethernetif_timer_id = osTimerNew(ethernetif_timer, osTimerPeriodic, NULL, &ethernet_timer_attrs);
  if (ethernetif_timer_id == NULL)
  {
    syslog_printf(SYSLOG_ERROR, "ETHERNETIF: cannot create timer");
  }

  // Create the Ethernet event flags.
  ethernetif_event_id = osEventFlagsNew(&ethernet_event_attrs);
  if (ethernetif_event_id == NULL)
  {
    syslog_printf(SYSLOG_ERROR, "ETHERNETIF: cannot create event flags");
  }

  // Initialize the Ethernet thread.
  ethernetif_thread_id = osThreadNew(ethernetif_thread, netif, &ethernet_thread_attrs);
  if (ethernetif_thread_id == NULL)
  {
    syslog_printf(SYSLOG_ERROR, "ETHERNETIF: cannot create thread");
  }

  return ERR_OK;
}

// Deinitialize the Ethernet interface.
err_t ethernetif_deinit(struct netif *netif)
{
  (void) netif;

  // Stop Ethernet MAC and DMA transmission and reception.
  HAL_ETH_Stop(&ethernetif_handle);

  return ERR_OK;
}

// Return the counts of packets send and received.
void ethernetif_counts(uint32_t *recv_count, uint32_t *recv_bytes, uint32_t *send_count, uint32_t *send_bytes)
{
  // Lock the Ethernet mutex to get the counts.
  osMutexAcquire(ethernetif_mutex_id, osWaitForever);

  // Return the count information.
  if (recv_count) *recv_count = ethernetif_recv_count;
  if (recv_bytes) *recv_bytes = ethernetif_recv_bytes;
  if (send_count) *send_count = ethernetif_send_count;
  if (send_bytes) *send_bytes = ethernetif_send_bytes;

  // Release the Ethernet mutex.
  osMutexRelease(ethernetif_mutex_id);
}

#if LWIP_PTPD
// Get the TX time associated with the packet buffer.
void ethernetif_get_tx_timestamp(struct pbuf *p)
{
  // Lock the Ethernet mutex.
  osMutexAcquire(ethernetif_mutex_id, osWaitForever);

  // Start without a DMA TX descriptor.
  __IO ETH_DMADescTypeDef *dma_tx_desc = NULL;

  // Find the DMA TX descriptor assocated with this packet buffer.
  // This is a onetime function to prevent issues with accidently
  // pointing to a recycled DMA TX descriptor.
  uint32_t index = ethernet_tx_tail;
  while (index != ethernet_tx_head)
  {
    // Is this the DMA TX descriptor we are interested in?
    if (p->time_nsec == ethernet_tx_entries[index].id)
    {
      // Get the DMA TX descriptor for use below.
      dma_tx_desc = ethernet_tx_entries[index].tx_desc;

      // This is a one time operation so clear the entry.
      ethernet_tx_entries[index].id = 0;
      ethernet_tx_entries[index].tx_desc = NULL;

      // We found the DMA TX descriptor.
      break;
    }

    // Increment to the next TX buffer.
    index = (index + 1) == ETH_TXBUFNB ? 0 : index + 1;
  }

  // Release the Ethernet mutex.
  osMutexRelease(ethernetif_mutex_id);

  // Fill in the default values.
  p->time_sec = 0;
  p->time_nsec = 0;

  // Did we find the dma tx descriptor?
  if (dma_tx_desc)
  {
    // Wait up to 20 millisecond for the DMA transfer to complete.
    for (uint32_t retry_count = 10; (retry_count > 0) && ((dma_tx_desc->Status & ETH_DMATXDESC_TTSS) != ETH_DMATXDESC_TTSS); --retry_count)
    {
      // Wait up to two milliseconds for a transfer to complete.
      osEventFlagsWait(ethernetif_event_id, ETHERNETIF_EVENT_TRANSMIT, osFlagsWaitAny, 2);
    }

    // Make sure after waiting we have the timestamp information.
    if (dma_tx_desc->Status & ETH_DMATXDESC_TTSS)
    {
      // Fill in the timestamp information.
      p->time_sec = dma_tx_desc->TimeStampHigh;
      p->time_nsec = subsecond_to_nanosecond(dma_tx_desc->TimeStampLow);
    }
    else
    {
      // Report timeount.
      outputf("ETHERNETIF: tx timestamp timeout\n");
    }
  }
}
#endif

// System configurable Ethernet preemptive interrupt priority value.
__WEAK uint32_t ethernetif_config_preempt_priority(void)
{
  return 8;
}

