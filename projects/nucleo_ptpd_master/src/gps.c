#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "cmsis_os2.h"
#include "rtx_os.h"
#include "hal_system.h"
#include "systime.h"
#include "syslog.h"
#include "outputf.h"
#include "extint.h"
#include "shell.h"
#include "gps.h"

// Venus638FLPx GPS Receiver
//
// https://www.talkunafraid.co.uk/2012/12/the-ntpi-accurate-time-with-a-raspberry-pi-and-venus638flpx/
//
// https://www.sparkfun.com/products/retired/11058
//
// This is the latest version of the SparkFun Venus GPS board; the smallest, most powerful,
// and most versatile GPS receiver SparkFun carries. It's based on the Venus638FLPx, the
// successor to the Venus634LPx. The Venus638FLPx outputs standard NMEA-0183 or SkyTraq Binary
// sentences at a default rate of 9600bps (adjustable to 115200bps), with update rates up to
// 20Hz!
//
// This board includes a SMA connector to attach an external antenna, headers for 3.3V
// serial data, NAV (lock) indication, Pulse-Per-Second output, and external Flash support.
// We've also provided solder jumpers to easily configure the power consumption, boot memory,
// and backup supply. This board requires a regulated 3.3V supply to operate; at full power
// the board uses up to 90mA, at reduced power it requires up to 60mA.
//
// GPS Peripheral Configuration
//
// GPIO Inputs
//
// PG14       USART6_TX      GPS_RX
// PG9        USART6_RX      GPS_TX
// PF15                      GPS_PPS
//

#define RX_BUFFER_SIZE          128
#define TX_BUFFER_SIZE          64
#define SEND_BUFFER_SIZE        64
#define PARSE_BUFFER_SIZE       256

#define GPS_EVENT_DATA          0x01u
#define GPS_EVENT_PPS           0x02u
#define GPS_EVENT_START         0x04u
#define GPS_EVENT_TIMER         0x08u
#define GPS_EVENT_MASK          0x0fu

// Information structure for sending data.
typedef struct gps_send_info_s
{
  uint8_t *buffer;
  uint16_t count;
} gps_send_info_t;

// Flag that indicates initial setting of the system clock.
static volatile bool gps_init_systime = false;

// USART receive buffer.
static uint8_t gps_rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t gps_rx_tail = 0;
static volatile uint16_t gps_rx_head = 0;

// USART transmit buffer.
static uint8_t gps_tx_buffer[TX_BUFFER_SIZE];
static volatile uint16_t gps_tx_tail = 0;
static volatile uint16_t gps_tx_head = 0;

// GPS NMEA and binary message parsing buffer.
static uint8_t gps_parse_buffer[PARSE_BUFFER_SIZE];
static uint16_t gps_parse_count = 0;
static uint32_t gps_parse_state = 0;
static uint8_t gps_binary_chksum = 0;
static uint16_t gps_binary_count = 0;

// GPS binary message send buffer.
static uint8_t gps_send_buffer[SEND_BUFFER_SIZE];

// GPS initialization state.
static uint32_t gps_init_state = 0;

// Thread for GPS processing.
static osMutexId_t gps_thread_id = NULL;

// Timer for GPS processing.
static osTimerId_t gps_timer_id = NULL;

// Message ack and nack values.
static uint8_t gps_message_ack = 0;
// XXX static uint8_t gps_message_nack = 0;

// Define the maximum rate at which we can adjust the system clock.
#define GPS_ADJ_FREQ_MAX      5120000

// Most recent GPS system time (GPS epoch -- nanoseconds since 1980).
static int64_t gps_sys_time = 0;
static int64_t gps_pps_time = 0;
static int64_t gps_start_time = 0;
static int64_t gps_parse_time = 0;
static int64_t gps_time_offset = 0;
static int32_t gps_clock_adjust = 0;
static int32_t gps_time_drift = 0;
static int32_t gps_sync_pgain = 2;
static int32_t gps_sync_igain = 16;

// Flag that indicates GPS configuration is complete.
static bool gps_configured = false;

// Configure NMEA binary message (Interval in seconds - 0 is disabled)
//  GGA   01    Fix information
//  GSA   01    Overall satellite data
//  GSV   00    Detailed satellite data
//  GLL   00    Latitue/longitude data
//  RMC   01    Recommended minimum data for GPS
//  VTG   01    Vector track and speed over the ground
//  ZDA   01    Date and time
static uint8_t gps_msg_config_nmea[9] =
{
  //    GGA   GSA   GSV   GLL   RMC   VTG   ZDA
  0x08, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00
};

// Set 1PPS mode to the GPS receiver.
static uint8_t gps_msg_config_1pps[3] =
{
  0x3E, 0x01, 0x00
};

void USART6_IRQHandler(void)
{
  // Is the USART_IT_RXNE interrupt enabled?
  if (LL_USART_IsEnabledIT_RXNE(USART6))
  {
    // Here we check if the system time is being initialized.  If so it means we want
    // to capture the first time data is flowing. Ideally would would check if the
    // USART_IT_RXNE flag with the following code:
    //
    // LL_USART_IsActiveFlag_RXNE(USART6)
    //
    // However, DMA reads the data faster than this interrupt can occur and the status
    // flag gets cancelled before it can be checked.  Therefore, if we see the 
    // USART_IT_RXNE is set in the USART config we assume this is the first data flowing.

    // We want the time of the first character being sent.
    gps_start_time = systime_get();

    // Signal the start of GPS data.
    osThreadFlagsSet(gps_thread_id, GPS_EVENT_START);

    // Disable the next RXNE interrupt.
    LL_USART_DisableIT_RXNE(USART6);
  }

  // Check for IDLE interrupt.
  if (((LL_USART_IsEnabledIT_IDLE(USART6)) && (LL_USART_IsActiveFlag_IDLE(USART6))))
  {
    // Clear the IDLE flag.
    LL_USART_ClearFlag_IDLE(USART6);

    // Get the current DMA data left to copy.
    gps_rx_head = RX_BUFFER_SIZE - LL_DMA_GetDataLength(DMA2, LL_DMA_STREAM_1);

    // Signal data in the input buffer.
    osThreadFlagsSet(gps_thread_id, GPS_EVENT_DATA);
  }

  // Check for the TXE interrupt.
  if (((LL_USART_IsEnabledIT_TXE(USART6))&& (LL_USART_IsActiveFlag_TXE(USART6))))
  {
    // Is there data to transmit?
    if (gps_tx_tail != gps_tx_head)
    {
      // Yes. Send the tail character.
      LL_USART_TransmitData9(USART6, (uint16_t) gps_tx_buffer[gps_tx_tail]);

      // Update the tail being careful to wrap at top.
      gps_tx_tail = (gps_tx_tail + 1) % TX_BUFFER_SIZE;
    }
    else
    {
      // No. Disable the TXE interrupt.
      LL_USART_DisableIT_TXE(USART6);
    }
  }
}

// Interrupt handler for DMA2 Stream 1.
void DMA2_Stream1_IRQHandler(void)
{
  // Handle the half transfer interrupt.
  if ((LL_DMA_IsEnabledIT_HT(DMA2, LL_DMA_STREAM_1) && LL_DMA_IsActiveFlag_HT1(DMA2)))
  {
    // Clear the DMA interrupt.
    LL_DMA_ClearFlag_HT1(DMA2);

    // Update the receive head.
    gps_rx_head = RX_BUFFER_SIZE / 2;

    // Signal data in the input buffer.
    osThreadFlagsSet(gps_thread_id, GPS_EVENT_DATA);
  }

  // Handle the transfer complete interrupt.
  if ((LL_DMA_IsEnabledIT_TC(DMA2, LL_DMA_STREAM_1) && LL_DMA_IsActiveFlag_TC1(DMA2)))
  {
    // Clear the DMA interrupt.
    LL_DMA_ClearFlag_TC1(DMA2);

    // Update the receive head.
    gps_rx_head = 0;

    // Signal data in the input buffer.
    osThreadFlagsSet(gps_thread_id, GPS_EVENT_DATA);
  }
}

// External interrupt line 5-9 handler.
static void gps_extint_handler(uint32_t extint_line)
{
  UNUSED(extint_line);

  // Set the time of the interrupt.
  gps_sys_time = systime_get();

  // Was time parsed?
  if (gps_parse_time != 0)
  {
    // Copy it as the PPS time.
    gps_pps_time = gps_parse_time;

    // Reset the parse time.
    gps_parse_time = 0;

    // Signal the PPS signal was timestamped.
    osThreadFlagsSet(gps_thread_id, GPS_EVENT_PPS);
  }
}

// Write data to GPS device. Overflows are silently ignored.
static void gps_write_data(uint8_t *buffer, uint16_t count)
{
  uint16_t next;

  // Set the next index being careful to wrap at top.
  next = (gps_tx_head + 1) % TX_BUFFER_SIZE;

  // Fill up as much of the buffer as possible.
  while ((count > 0) && (next != gps_tx_tail))
  {
    // Insert into the transmit buffer.
    gps_tx_buffer[gps_tx_head] = *buffer;

    // Update the head to the next index.
    gps_tx_head = next;

    // Set the next index being careful to wrap at top.
    next = (gps_tx_head + 1) % TX_BUFFER_SIZE;

    // Increment the buffer and decrement the count.
    buffer += 1;
    count -= 1;
  }

  // Enable the TX interrupt.
  LL_USART_EnableIT_TXE(USART6);
}

// Initialize GPS GPIO, UART and DMA peripherals.
static void gps_peripheral_init(void)
{
  LL_DMA_InitTypeDef dma_init;
  LL_EXTI_InitTypeDef exti_init;
  LL_GPIO_InitTypeDef gpio_init;
  LL_USART_InitTypeDef usart_init;

  // Enable peripheral clocks.
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOF);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOG);
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART6);

  // Enable global interrupts for GPS USART.
  uint32_t grouping = NVIC_GetPriorityGrouping();
  uint32_t priority = NVIC_EncodePriority(grouping, 3, 0);
  NVIC_SetPriority(USART6_IRQn, priority);
  NVIC_EnableIRQ(USART6_IRQn);

  // Enable global interrupts for DMA stream.
  priority = NVIC_EncodePriority(grouping, 2, 0);
  NVIC_SetPriority(DMA2_Stream1_IRQn, priority);
  NVIC_EnableIRQ(DMA2_Stream1_IRQn);

  // Configure GPIO pin for PPS signal from GPS.
  LL_GPIO_StructInit(&gpio_init);
  gpio_init.Pin = LL_GPIO_PIN_15;
  gpio_init.Mode = LL_GPIO_MODE_INPUT;
  gpio_init.Pull = LL_GPIO_PULL_DOWN;
  gpio_init.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOF, &gpio_init);

  // Set the EXTI_Line15 callback function.
  extint_set_callback(LL_EXTI_LINE_15, gps_extint_handler);

  // GPIOC will be used for for EXTI_Line15.
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTF, LL_SYSCFG_EXTI_LINE15);

  // Enable EXTI_Line8 for rising edge change.
  LL_EXTI_StructInit(&exti_init);
  exti_init.Line_0_31 = LL_EXTI_LINE_15;
  exti_init.Mode = LL_EXTI_MODE_IT;
  exti_init.Trigger = LL_EXTI_TRIGGER_RISING;
  exti_init.LineCommand = ENABLE;
  LL_EXTI_Init(&exti_init);

  // Configure GPIO pin for USART6 TX.
  LL_GPIO_StructInit(&gpio_init);
  gpio_init.Pin = LL_GPIO_PIN_9;
  gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.Pull = LL_GPIO_PULL_UP;
  gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
  gpio_init.Alternate = LL_GPIO_AF_8;
  LL_GPIO_Init(GPIOG, &gpio_init);

  // Configure GPIO pin for USART6 RX.
  LL_GPIO_StructInit(&gpio_init);
  gpio_init.Pin = LL_GPIO_PIN_14;
  gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.Pull = LL_GPIO_PULL_UP;
  gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
  gpio_init.Alternate = LL_GPIO_AF_8;
  LL_GPIO_Init(GPIOG, &gpio_init);

  // USARTx configured as follows: 
  // 9600 baud, 8 Bits, 1 Stop, No Parity, 
  // No Flow Control, Receive and Transmit Enabled.
  LL_USART_StructInit(&usart_init);
  usart_init.BaudRate = 9600;
  LL_USART_Init(USART6, &usart_init);

#ifdef STM32F7
  // Disable overrun detection.
  LL_USART_DisableOverrunDetect(USART6);
#endif

  // Disable USART interrupts.
  LL_USART_DisableIT_CTS(USART6);
  LL_USART_DisableIT_LBD(USART6);
  LL_USART_DisableIT_TXE(USART6);
  LL_USART_DisableIT_TC(USART6);
  LL_USART_DisableIT_RXNE(USART6);
  LL_USART_DisableIT_IDLE(USART6);
  LL_USART_DisableIT_PE(USART6);
  LL_USART_DisableIT_ERROR(USART6);

  // Configure DMA for USART6 RX, DMA2, Stream 1, Channel 5.
  LL_DMA_StructInit(&dma_init);
  dma_init.Channel = LL_DMA_CHANNEL_5;
  dma_init.NbData = RX_BUFFER_SIZE;
  dma_init.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
  dma_init.Mode = LL_DMA_MODE_CIRCULAR;
  dma_init.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
  dma_init.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_1_2;
  dma_init.MemoryOrM2MDstAddress = (uint32_t) gps_rx_buffer;
  dma_init.MemBurst = LL_DMA_MBURST_SINGLE;
  dma_init.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
  dma_init.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
  dma_init.PeriphOrM2MSrcAddress = (uint32_t) &USART6->DR;
  dma_init.PeriphBurst = LL_DMA_PBURST_SINGLE;
  dma_init.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
  dma_init.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
  dma_init.Priority = LL_DMA_PRIORITY_HIGH;
  LL_DMA_Init(DMA2, LL_DMA_STREAM_1, &dma_init);

  // Enable the DMA2, Stream 1 half transfer and transfer complete interrupt.
  LL_DMA_EnableIT_HT(DMA2, LL_DMA_STREAM_1);
  LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_1);

  // Enable DMA Stream 1.
  LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_1);

  // Enable USART receive DMA.
  LL_USART_EnableDMAReq_RX(USART6);

  // Enable the USART IDLE interrupt.
  LL_USART_EnableIT_IDLE(USART6);

  // Enable the USART.
  LL_USART_Enable(USART6);
}

// Process the GPS GPZDA NMEA sentence.
static void gps_process_gpzda_sentence(int argc, char **argv)
{
  int hour;
  int min;
  int sec;
  int msec;
  struct tm gps_tm;
  time_t seconds1970;

  // Make sure we have the time value.
  if (argc < 7) return;

  // Parse the hours, minutes, seconds and milliseconds field.
  if (sinputf(argv[1], "%2u%2u%2u.%3u", &hour, &min, &sec, &msec) != 4) return;

  // Fill in the time structure.
  memset(&gps_tm, 0, sizeof(gps_tm));
  gps_tm.tm_sec = sec;
  gps_tm.tm_min = min;
  gps_tm.tm_hour = hour;
  gps_tm.tm_mday = atoi(argv[2]);
  gps_tm.tm_mon = atoi(argv[3]) - 1;
  gps_tm.tm_year = atoi(argv[4]) - 1900;

  // Sanity check the date values.  This is to avoid a parsing error
  // causing the clock to be set to an insane value.  Members tm_wday
  // and tm_yday are ignored by mktime.
  if ((gps_tm.tm_sec < 0) || (gps_tm.tm_sec > 59)) return;
  if ((gps_tm.tm_min < 0) || (gps_tm.tm_min > 59)) return;
  if ((gps_tm.tm_hour < 0) || (gps_tm.tm_hour > 23)) return;
  if ((gps_tm.tm_mday < 1) || (gps_tm.tm_mday > 31)) return;
  if ((gps_tm.tm_mon < 0) || (gps_tm.tm_mon > 11)) return;
  if ((gps_tm.tm_year < 100) || (gps_tm.tm_year > 140)) return;

  // Get the seconds since 1970 using mktime().
  seconds1970 = mktime(&gps_tm);

  // Determine the GPS parsed time using mktime().
  gps_parse_time = ((((int64_t) seconds1970) - 315964800) * 1000000000) + (msec * 1000000);

  // We actually want the time of the next PPS so we add 1 second to the PPS time.
  gps_parse_time += 1000000000;

  // If system time is not yet initialized, we want to capture the next time data
  // starts so we can initialize the system time from the parsed time. 
  // Note: There is an important assumption here that GPZDA is the last sentence
  // in the group of NMEA sentences passed from the GPS device each second. This
  // assumption seems to hold true for the Venus638FLPx GPS Receiver, but may not
  // for other receivers.
  if (!gps_init_systime)
  {
    // Enable the next RXNE interrupt.
    LL_USART_EnableIT_RXNE(USART6);
  }
}

// Parse out the next comma delinated word from a string.
// str    Pointer to pointer to the string
// word   Pointer to pointer of next word.
// Returns 0:Failed, 1:Successful
static int gps_nmea_parse(char **str, char **word)
{
  // Skip leading spaces.
  while (**str && isspace((unsigned char) **str)) (*str)++;

  // Set the word.
  *word = *str;

  // Skip non-comma characters.
  while (**str && (**str != ',')) (*str)++;

  // Null terminate the word.
  if (**str) *(*str)++ = 0;

  return (*str != *word) ? 1 : 0;
}

// Process the GPS NMEA sentence.
static void gps_process_nmea_sentence(char *sentence)
{
  int i;
  int argc = 0;
  char *argv[16];

  // XXX outputf("%s\n", sentence);

  // We avoid further processing non-GPZDA sentences.
  if (strncmp(sentence, "$GPZDA", 6) == 0)
  {
    // Parse the NMEA sentence into comma deliminated fields.
    for (i = 0; i < (sizeof(argv) / sizeof(char *)); ++i)
    {
      gps_nmea_parse(&sentence, &argv[i]);
      if (*argv[i] != 0) ++argc;
    }

    // Is this an NMEA sentence we care about?
    if (strcmp(argv[0], "$GPZDA") == 0)
    {
      // Process teh GPZDA sentence.
      gps_process_gpzda_sentence(argc, argv);
    }
  }
}

// Process the binary message.
static void gps_process_binary_message(uint8_t *buffer, uint16_t count)
{
  // Determine the message type.
  if ((count == 2) && (buffer[0] == 0x83))
  {
    // ACK message. Set the request message ID.
    gps_message_ack = buffer[1];
    // XXX outputf("ack received 0x%02x\n", buffer[1]);
  }
#if 0
  else if ((count == 2) && (buffer[0] == 0x84))
  {
    // NACK message. Set the request message ID.
    gps_message_nack = buffer[1];
    // XXX outputf("nack received 0x%02x\n", buffer[1]);
  }
#endif
  else
  {
    // XXX outputf("unknown message received 0x%02x %d\n", buffer[0], count);
  }
}

// Parse the GPS data buffer looking for known data.
static void gps_parse(uint8_t *buffer, uint16_t count)
{
  // Parse the buffer a single character at a time.
  while (count)
  {
    // Handle the parsing state.
    switch (gps_parse_state)
    {
      case 0:
        // Is this the start of an NMEA sentence or binary message?
        if (*buffer == '$')
        {
          // Reset the NMEA sentence parsing information.
          gps_parse_buffer[0] = *buffer;
          gps_parse_count = 1;

          // Move to the next state to parse the NMEA sentence.
          gps_parse_state = 1;
        }
        else if (*buffer == 0xA0)
        {
          // Reset the binary parsing information.
          gps_binary_count = 0;
          gps_binary_chksum = 0;
          gps_parse_count = 0;

          // Move to the next state to parse the binary message.
          gps_parse_state = 2;
        }
        break;

      case 1:
        // Is this the end the NMEA sentence?
        if ((*buffer == '\n') || (*buffer == '\r'))
        {
          // Null terminate the buffer.
          gps_parse_buffer[gps_parse_count] = 0;

          // Process the NMEA statement as a string buffer.
          gps_process_nmea_sentence((char *) gps_parse_buffer);

          // Return to the start state.
          gps_parse_state = 0;
        }
        else if (*buffer == 0xA0)
        {
          // Reset the binary parsing information.
          gps_binary_count = 0;
          gps_binary_chksum = 0;

          // Move to the next state to parse the binary message.
          gps_parse_state = 2;
        }
        else
        {
          // Make sure we are parsing a properly formatted NMEA sentence.
          if ((*buffer > ' ') && (*buffer <= '~'))
          {
            // Be careful not to overflow the buffer.
            if (gps_parse_count < (PARSE_BUFFER_SIZE - 1))
            {
              // Insert the character into the NMEA buffer.
              gps_parse_buffer[gps_parse_count] = *buffer;
              gps_parse_count += 1;
            }
            else
            {
              // Buffer overflow! Return to the start state.
              gps_parse_state = 0;
            }
          }
          else
          {
            // Invalid character. Return to the start state.
            gps_parse_state = 0;
          }
        }
        break;

      case 2:
        // Is this the second binary message byte?
        if (*buffer == 0xA1)
        {
          // Move to the next state.
          gps_parse_state = 3;
        }
        else
        {
          // Return to default state:
          gps_parse_state = 0;
        }
        break;

      case 3:
        // Get the first byte of the payload length.
        gps_binary_count = ((uint16_t) *buffer) << 8;

        // Move to the next state.
        gps_parse_state = 4;
        break;

      case 4:
        // Get the next byte of the payload length.
        gps_binary_count |= (uint16_t) *buffer;

        // Can we parse a binary message of this length?
        if (gps_binary_count > (PARSE_BUFFER_SIZE - 1))
        {
          // Buffer overflow. Return to default state:
          gps_parse_state = 0;
        }
        else
        {
          // Move to the next state.
          gps_parse_state = 5;
        }
        break;

      case 5:
        // Be careful not to overflow the parse buffer.
        if (gps_parse_count < (PARSE_BUFFER_SIZE - 1))
        {
          // Add the character to the parse buffer.
          gps_parse_buffer[gps_parse_count] = *buffer;

          // Include the character in the checksum.
          gps_binary_chksum ^= *buffer;

          // Increment the parse count.
          gps_parse_count += 1;

          // Decrement the count.
          gps_binary_count -= 1;

          // Have we reached the end?
          if (gps_binary_count == 0)
          {
            // Move to the next state.
            gps_parse_state = 6;
          }
        }
        else
        {
          // Buffer overflow. Return to default state:
          gps_parse_state = 0;
        }
        break;

      case 6:
        // Does the checksum match?
        if (gps_binary_chksum == *buffer)
        {
          // Move to the next state.
          gps_parse_state = 7;
        }
        else
        {
          // Return to default state:
          gps_parse_state = 0;
        }
        break;

      case 7:
        // Is the byte before the end of the binary message byte?
        if (*buffer == 0x0D)
        {
          // Move to the next state.
          gps_parse_state = 8;
        }
        else
        {
          // Return to default state:
          gps_parse_state = 0;
        }
        break;

      case 8:
        // Is this end of the binary message byte?
        if (*buffer == 0x0A)
        {
          // Everything looks good, process the binary data.
          gps_process_binary_message(gps_parse_buffer, gps_parse_count);
        }

        // Return to default state:
        gps_parse_state = 0;
        break;
    }

    // Decrement the count and increment the buffer.
    count -= 1;
    buffer += 1;
  }
}

// Process GPS data buffer.
static void gps_process_data(uint8_t *buffer, uint16_t count)
{
  // Sanity check the count.
  if (count == 0) return;

  // Parse the buffer looking for NMEA or binary data.
  gps_parse(buffer, count);
}

// Send a properly formatted binary message to the GPS device.
static void gps_send_message(uint8_t *buffer, uint16_t count)
{
  uint16_t i;
  uint8_t chksum = 0;

  // Make sure the message isn't too big.
  if (count > (SEND_BUFFER_SIZE - 7)) return;

  // Fill in the first part of the binary message.
  gps_send_buffer[0] = 0xA0;
  gps_send_buffer[1] = 0xA1;

  // Fill in the message length.
  gps_send_buffer[2] = (uint8_t) ((count >> 8) & 0xFF);
  gps_send_buffer[3] = (uint8_t) (count & 0xFF);

  // Fill in the buffer and compute the checksum.
  for (i = 0; i < count; ++i)
  {
    // Put the data in the checksum.
    chksum ^= buffer[i];

    // Put in the buffer.
    gps_send_buffer[4 + i] = buffer[i];
  }

  // Fill in the payload checksum.
  gps_send_buffer[4 + count] = chksum;

  // Fill in the last part of the binary message.
  gps_send_buffer[4 + count + 1] = 0x0D;
  gps_send_buffer[4 + count + 2] = 0x0A;

  // Write the message to the GPS serial port.
  gps_write_data(gps_send_buffer, count + 7);
}

// Process the GPS timer.
static void gps_process_timer(void)
{
  // Handle the init state.
  switch (gps_init_state)
  {
    case 0:
      // Move to the next state.
      gps_init_state = 1;
      break;

    case 1:
      // Configure the GPS NMEA messages.
      gps_send_message(gps_msg_config_nmea, sizeof(gps_msg_config_nmea));

      // Move to the next state.
      gps_init_state = 2;
      break;

    case 2:
      // Did we receive an ACK for the configure GPS NMEA messages?
      if (gps_message_ack == 0x08)
      {
        // Yes. Move to the next init state.
        gps_init_state = 3;
      }
      else
      {
        // No. Try configuration again.
        gps_init_state = 1;
      }
      break;

    case 3:
      // Configure the GPS 1PPS signal.
      gps_send_message(gps_msg_config_1pps, sizeof(gps_msg_config_1pps));

      // Move to the next state.
      gps_init_state = 4;
      break;

    case 4:
      // Did we receive an ACK for the configure GPS 1PPS signal?
      if (gps_message_ack == 0x3E)
      {
        // Yes. Move to the next init state.
        gps_init_state = 5;
      }
      else
      {
        // No. Try configuration again.
        gps_init_state = 3;
      }
      break;

    case 5:
      // We are configured so stop the periodic timer.
      osTimerStop(gps_timer_id);

      // Set the GPS configured flag.
      gps_configured = true;

      // Log that GPS is now initialized.
      syslog_printf(SYSLOG_NOTICE, "GPS: successfully configured");
      break;
  }
}

// GPS periodic timer handler. This is processed on the timer thread.
static void gps_timer_handler(void *arg)
{
  // Set the periodic timer event.
  if (gps_thread_id) osThreadFlagsSet(gps_thread_id, GPS_EVENT_TIMER);
}

// Initialize the high-precision system clock against the initial
// parsed GPZDA sentence. We do this to get a rough estimate of the
// system time before we start receiving the GPS PPS signal.
static void gps_time_init(void)
{
  int64_t delta;
  int64_t init_time;
  char buffer[32];

  // Ignore if system time already initialized.  This can
  // happen if we have already quickly receive a PPS signal.
  if (gps_init_systime) return;

  // Start with the previous parsed time.
  init_time = gps_parse_time;

  // Determine the current delta from the start time.
  delta = systime_get() - gps_start_time;

  // To be used to initialize system time, the delta should be no more
  // than a second. If not used, we'll try again on the next go around.
  if (delta < 1000000000)
  {
    // Apply the delta to the parsed GPS time.
    init_time += delta;

    // Set the clock to the parsed time. We don't have a PPS signal
    // yet so this will have to do for now.
    systime_set(init_time);

    // Mark the time as initialized.
    gps_init_systime = true;

    // Get the date from system time.
    systime_str(buffer, sizeof(buffer));

    // Log the time being set.
    syslog_printf(SYSLOG_NOTICE, "GPS: setting %s", buffer);
  }
}

// Synchronize the high-precision system clock against the GPS PPS signal.
static void gps_time_sync(void)
{
  int32_t seconds;
  int32_t nseconds;
  char buffer[32];

  // Sanity check we have both system and GPS time at time of the PPS.
  if ((gps_sys_time == 0) || (gps_pps_time == 0))
    return;

  // Count GPS PPS processing.
  // XXX count_add(COUNT_GPS_PPS);

  // Determine clock offset from PPS.
  gps_time_offset = gps_pps_time - gps_sys_time;

  // Determine the seconds and nanoseconds components of the time offset.
  seconds = (int32_t) (gps_time_offset / 1000000000);
  nseconds = (int32_t) (gps_time_offset - (seconds * 1000000000));

  // Is clock offset from PPS greater than 100 milliseconds?
  if ((seconds != 0) || (abs(nseconds) > 100000000))
  {
    // Determine the current delta from the system time.
    int64_t delta = systime_get() - gps_sys_time;

    // Apply the delta to the GPS time of the PPS.
    gps_pps_time += delta;

    // Set the clock to the actual time. Hopefully it should now
    // be less than 100ms off so synchronization can commence.
    systime_set(gps_pps_time);

    // Mark the time as initialized.
    gps_init_systime = true;

    // Get the date from system time.
    systime_str(buffer, sizeof(buffer));

    // Log the time being set.
    syslog_printf(SYSLOG_NOTICE, "GPS: setting %s", buffer);

    // XXX outputf("offset: %d secs %d nsecs\n", seconds, nseconds);
    // XXX outputf("resetting system time\n", nseconds);
  }
  else
  {
    // Implement the PI controller to synchronize time.

    // Keep track of accumulated offset (aka drift).
    gps_time_drift += nseconds / gps_sync_igain;

    // Clamp the accumlated offset to maximum adjustment frequency.
    if (gps_time_drift > GPS_ADJ_FREQ_MAX)
      gps_time_drift = GPS_ADJ_FREQ_MAX;
    else if (gps_time_drift < -GPS_ADJ_FREQ_MAX)
      gps_time_drift = -GPS_ADJ_FREQ_MAX;

    // Determine clock adjustment from PI controller output.  The
    // clock adjustment is a value that represents the frequency
    // adjustment of the clock (in parts per billion, ppb).
    gps_clock_adjust = (nseconds / gps_sync_pgain) + gps_time_drift;

    // Adjust the system clock.
    systime_adjust(gps_clock_adjust);

    // XXX outputf("offset: %d adjust: %d\n", nseconds, clock_adjust);
  }

  // Reset the GPS and SYS time.
  gps_pps_time = gps_sys_time = 0;
}

// GPS shell utility.
static bool gps_shell_gps(int argc, char **argv)
{
  char sign;
  char buffer[32];
  int64_t offset_secs;

  // Get the date from system time.
  systime_str(buffer, sizeof(buffer));

  // Print the date.
  shell_printf("DATE: %s\n", buffer);

  // Offset from PPS.
  offset_secs = gps_time_offset / 1000000000;
  if (offset_secs != 0)
  {
    shell_printf("OFFSET: %d sec\n", (int32_t) offset_secs);
  }
  else
  {
    shell_printf("OFFSET: %d nsec\n", (int32_t) gps_time_offset);
  }

  // Drift from PPS.
  sign = ' ';
  if (gps_time_drift > 0) sign = '+';
  if (gps_time_drift < 0) sign = '-';
  shell_printf("DRIFT: %c%d.%03d ppm\n", sign, abs(gps_time_drift / 1000), abs(gps_time_drift % 1000));

  return true;
}

// GPS processing thread.
static void gps_thread(void *arg)
{
  uint16_t head;

  // Initialzie the GPS peripherals for processing.
  gps_peripheral_init();

  // Start the periodic timer callback every 1000ms.
  osTimerStart(gps_timer_id, tick_from_milliseconds(1000));

  // Loop forever.
  for (;;)
  {
    // Wait for an event to be set.
    uint32_t events = osThreadFlagsWait(GPS_EVENT_MASK, osFlagsWaitAny, osWaitForever);

    // Ignore events flagged with an error.
    if ((events & 0x80000000) != 0x80000000)
    {
      // Is this a serial data event?
      if (events & GPS_EVENT_DATA)
      {
        // Get the current head of the receive buffer.
        head = gps_rx_head;

        // Is the head ahead of the tail.
        if (head > gps_rx_tail)
        {
          // Yes. We can process a single continguous buffer.
          gps_process_data(&gps_rx_buffer[gps_rx_tail], head - gps_rx_tail);
        }
        else if (head < gps_rx_tail)
        {
          // No. Process the first part of the discontiguous buffer.
          gps_process_data(&gps_rx_buffer[gps_rx_tail], RX_BUFFER_SIZE - gps_rx_tail);

          // Process the second part of the discontiguous buffer.
          if (head > 0) gps_process_data(&gps_rx_buffer[0], head);
        }

        // Update the tail to reflect the processed data.
        gps_rx_tail = head;
      }

      // Is this a GPS PPS event?
      if (events & GPS_EVENT_PPS)
      {
        // Sychronize the clock against the PPS signal.
        gps_time_sync();
      }

      // Is this a start of GPS data event?
      if (events & GPS_EVENT_START)
      {
        // Initialize the clock against the parsed GPS time.
        gps_time_init();
      }

      // Is this a periodic timer event?
      if (events & GPS_EVENT_TIMER)
      {
        // Process the timer event.
        gps_process_timer();
      }
    }
  }
}

// Initialize GPS UART peripheral.
void gps_init(void)
{
  // Reset the GPS configured flag.
  gps_configured = false;

  // Static control blocks.
  static uint32_t gps_timer_cb[osRtxTimerCbSize/4U] __attribute__((section(".bss.os.timer.cb")));
  static uint32_t gps_thread_cb[osRtxThreadCbSize/4U] __attribute__((section(".bss.os.thread.cb")));

  // GPS timer attributes.
  osTimerAttr_t gps_timer_attrs =
  {
    .name = "gps",
    .attr_bits = 0U,
    .cb_mem = gps_timer_cb,
    .cb_size = sizeof(gps_timer_cb)
  };

  // GPS thread attributes.
  osThreadAttr_t gps_thread_attrs =
  {
    .name = "gps",
    .attr_bits = 0U,
    .cb_mem = gps_thread_cb,
    .cb_size = sizeof(gps_thread_cb),
    .priority = osPriorityNormal
  };

  // Create the GPS timer.
  gps_timer_id = osTimerNew(gps_timer_handler, osTimerPeriodic, NULL, &gps_timer_attrs);
  if (!gps_timer_id)
  {
    // Log an error.
    syslog_printf(SYSLOG_ERROR, "GPS: error creating periodic timer");
    return;
  }

  // Create the GPS thread.
  gps_thread_id = osThreadNew(gps_thread, NULL, &gps_thread_attrs);
  if (!gps_thread_id)
  {
    // Report an error.
    syslog_printf(SYSLOG_ERROR, "GPS: error creating processing thread");
    return;
  }

  // Add the GPS shell function.
  shell_add_command("gps", gps_shell_gps);
}

// Returns true once GPS processing is in a known to be properly initialized.
bool gps_is_init(void)
{
  return gps_configured;
}

