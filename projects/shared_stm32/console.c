#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "cmsis_os2.h"
#include "rtx_os.h"
#include "hal_system.h"
#include "shell.h"
#include "syslog.h"
#include "ntshell.h"
#include "ntlibc.h"
#include "console.h"

// Console event flag values.
#define CONSOLE_EVENT_RECV          0x0001u
#define CONSOLE_EVENT_SEND          0x0002u

// Console thread event flag values.
#define CONSOLE_THREAD_EVENT_IO     0x40000000u

#if defined(USE_STM32F7XX_NUCLEO_144)
//
// USART3 Console Hardware Configuration
//
// GPIO Inputs
//
// PD8         SERIAL_TX
// PD9         SERIAL_RX
//
#define CONSOLE_PERIPH_GPIO       LL_AHB1_GRP1_PERIPH_GPIOD
#define CONSOLE_PERIPH_USART      LL_APB1_GRP1_PERIPH_USART3
#define CONSOLE_PERIPH_ENABLECLK  LL_APB1_GRP1_EnableClock
#define CONSOLE_GPIO_AF           LL_GPIO_AF_7
#define CONSOLE_USART             USART3
#define CONSOLE_IRQ               USART3_IRQn
#define CONSOLE_GPIO              GPIOD
#define CONSOLE_TX_PIN            LL_GPIO_PIN_8
#define CONSOLE_RX_PIN            LL_GPIO_PIN_9
#define CONSOLE_TX_PINSOURCE      LL_GPIO_PIN_8
#define CONSOLE_RX_PINSOURCE      LL_GPIO_PIN_9
#endif

#if defined(USE_STM32F4XX_NUCLEO_144)
//
// USART3 Console Hardware Configuration
//
// GPIO Inputs
//
// PD8         SERIAL_TX
// PD9         SERIAL_RX
//
#define CONSOLE_PERIPH_GPIO       LL_AHB1_GRP1_PERIPH_GPIOD
#define CONSOLE_PERIPH_USART      LL_APB1_GRP1_PERIPH_USART3
#define CONSOLE_PERIPH_ENABLECLK  LL_APB1_GRP1_EnableClock
#define CONSOLE_GPIO_AF           LL_GPIO_AF_7
#define CONSOLE_USART             USART3
#define CONSOLE_IRQ               USART3_IRQn
#define CONSOLE_GPIO              GPIOD
#define CONSOLE_TX_PIN            LL_GPIO_PIN_8
#define CONSOLE_RX_PIN            LL_GPIO_PIN_9
#define CONSOLE_TX_PINSOURCE      LL_GPIO_PIN_8
#define CONSOLE_RX_PINSOURCE      LL_GPIO_PIN_9
#endif

#if defined(USE_STM32F4_DISCOVERY)
//
// USART6 Console Hardware Configuration
//
// GPIO Inputs
//
// PC6         SERIAL_TX
// PC7         SERIAL_RX
//
#define CONSOLE_PERIPH_GPIO       LL_AHB1_GRP1_PERIPH_GPIOC
#define CONSOLE_PERIPH_USART      LL_APB2_GRP1_PERIPH_USART6
#define CONSOLE_PERIPH_ENABLECLK  LL_APB2_GRP1_EnableClock
#define CONSOLE_GPIO_AF           LL_GPIO_AF_8
#define CONSOLE_USART             USART6
#define CONSOLE_IRQ               USART6_IRQn
#define CONSOLE_GPIO              GPIOC
#define CONSOLE_TX_PIN            LL_GPIO_PIN_6
#define CONSOLE_RX_PIN            LL_GPIO_PIN_7
#define CONSOLE_TX_PINSOURCE      LL_GPIO_PIN_6
#define CONSOLE_RX_PINSOURCE      LL_GPIO_PIN_7
#endif

#if defined(USE_STM32F405_OLIMEX)
//
// USART6 Console Hardware Configuration
//
// GPIO Inputs
//
// PC6         SERIAL_TX
// PC7         SERIAL_RX
//
#define CONSOLE_PERIPH_GPIO       LL_AHB1_GRP1_PERIPH_GPIOC
#define CONSOLE_PERIPH_USART      LL_APB2_GRP1_PERIPH_USART6
#define CONSOLE_PERIPH_ENABLECLK  LL_APB2_GRP1_EnableClock
#define CONSOLE_GPIO_AF           LL_GPIO_AF_8
#define CONSOLE_USART             USART6
#define CONSOLE_IRQ               USART6_IRQn
#define CONSOLE_GPIO              GPIOC
#define CONSOLE_TX_PIN            LL_GPIO_PIN_6
#define CONSOLE_RX_PIN            LL_GPIO_PIN_7
#define CONSOLE_TX_PINSOURCE      LL_GPIO_PIN_6
#define CONSOLE_RX_PINSOURCE      LL_GPIO_PIN_7
#endif

// Console thread id.
static osThreadId_t console_thread_id = NULL;

// Console event flags id.
static osEventFlagsId_t console_event_id = NULL;

// Console mutex protection.
static osMutexId_t console_mutex_id = NULL;

#define CONSOLE_SEND_BUFFER_SIZE    32
#define CONSOLE_RECV_BUFFER_SIZE    32

// Send and receive buffers.
static char console_send_buffer[CONSOLE_SEND_BUFFER_SIZE];
static char console_recv_buffer[CONSOLE_RECV_BUFFER_SIZE];
static volatile size_t console_send_head = 0;
static volatile size_t console_send_tail = 0;
static volatile size_t console_recv_head = 0;
static volatile size_t console_recv_tail = 0;

// Weak global for reboot of system.
__WEAK void reboot_system(void)
{
  // Does nothing. Expected to be overriden by actual reboot system call.
}

// Weak global for shutdown of system.
__WEAK void shutdown_system(void)
{
  // Does nothing. Expected to be overriden by actual shutdown system call.
}

#if defined(USE_STM32F7XX_NUCLEO_144)
void USART3_IRQHandler(void)
{
  // Keep track if the interrupt was handled.
  bool int_handled = false;

  // Check for the transmitter empty (TXE) condition.
  if (((CONSOLE_USART->CR1 & USART_CR1_TXEIE) && (CONSOLE_USART->ISR & USART_ISR_TXE)))
  {
    // Are there characters to send?
    if (console_send_tail != console_send_head)
    {
      // Put the next character in the transmit buffer which clears the interrupt.
      CONSOLE_USART->TDR = (uint32_t) console_send_buffer[console_send_tail];

      // Increment the tail.
      console_send_tail = (console_send_tail + 1) % CONSOLE_SEND_BUFFER_SIZE;
    }

    // Disable transmitter empty if all data is sent.
    if (console_send_tail == console_send_head)
    {
      // Disable the transmit buffer empty interrupt.
      CONSOLE_USART->CR1 &= ~(USART_CR1_TXEIE);
    }

    // Signal data has been sent.
    osEventFlagsSet(console_event_id, CONSOLE_EVENT_SEND);

    // Interrupt was handled.
    int_handled = true;
  }

  // Check for the receiver not empty (RXNE) condition.
  if (((CONSOLE_USART->CR1 & USART_CR1_RXNEIE) && (CONSOLE_USART->ISR & USART_ISR_RXNE)))
  {
    // Check for various errors.
    uint32_t recv_errors = CONSOLE_USART->ISR & (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE);

    // Read the received character which will clear the interrupt.
    char ch = (char) (CONSOLE_USART->RDR & 0x7f);

    // Reset any error flags after the read.
    if (!recv_errors) CONSOLE_USART->ICR = (USART_ICR_PECF | USART_ICR_FECF | USART_ICR_ORECF | USART_ICR_NCF);

    // Determine the next head index.
    size_t next_recv_head = (console_recv_head + 1) % CONSOLE_RECV_BUFFER_SIZE;

    // Make sure receiving this character doesn't overflow the receive buffer.
    if (next_recv_head != console_recv_tail)
    {
      // Insert the character into the receive buffer.
      console_recv_buffer[console_recv_head] = ch;

      // Update the receive head.
      console_recv_head = next_recv_head;
    }

    // Signal the receive event.
    osEventFlagsSet(console_event_id, CONSOLE_EVENT_RECV);

    // Notify the console thread of I/O available. This is a more general purpose
    // event specifically on the console thread to indicate I/O has taken place.
    if (console_thread_id) osThreadFlagsSet(console_thread_id, CONSOLE_THREAD_EVENT_IO);

    // Interrupt was handled.
    int_handled = true;
  }

  // If the interrupt was not handled, attempt to clear it by reading from the input register
  // and clearing all potential error flags.
  if (!int_handled)
  {
    // Check for various errors.
    uint32_t recv_errors = CONSOLE_USART->ISR & (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE);

    // Read the received character which will clear the interrupt.
    char ch = (char) (CONSOLE_USART->RDR & 0x7f);
    (void) ch;

    // Reset any error flags after the read.
    if (!recv_errors) CONSOLE_USART->ICR = (USART_ICR_PECF | USART_ICR_FECF | USART_ICR_ORECF | USART_ICR_NCF);
  }
}
#endif

#if defined(USE_STM32F4XX_NUCLEO_144)
void USART3_IRQHandler(void)
{
  // Check for the TXE interrupt.
  if (((LL_USART_IsEnabledIT_TXE(CONSOLE_USART)) && (LL_USART_IsActiveFlag_TXE(CONSOLE_USART))))
  {
    // Are there characters to send?
    if (console_send_tail != console_send_head)
    {
      // Put the next character in the transmit buffer which clears the interrupt.
      LL_USART_TransmitData9(CONSOLE_USART, (uint16_t) console_send_buffer[console_send_tail]);

      // Increment the tail.
      console_send_tail = (console_send_tail + 1) % CONSOLE_SEND_BUFFER_SIZE;
    }

    // Disable transmitter empty if all data is sent.
    if (console_send_tail == console_send_head)
    {
      // Disable the transmit buffer empty interrupt.
      LL_USART_DisableIT_TXE(CONSOLE_USART);
    }

    // Signal the send event.
    osEventFlagsSet(console_event_id, CONSOLE_EVENT_SEND);
  }

  // Is the USART_IT_RXNE interrupt enabled?
  if (((LL_USART_IsEnabledIT_RXNE(CONSOLE_USART)) && (LL_USART_IsActiveFlag_RXNE(CONSOLE_USART))))
  {
    // Read the received character which will clear the interrupt.
    char ch = (char) (LL_USART_ReceiveData9(CONSOLE_USART) & 0x7f);

    // Determine the next head index.
    size_t next_recv_head = (console_recv_head + 1) % CONSOLE_RECV_BUFFER_SIZE;

    // Make sure receiving this character doesn't overflow the receive buffer.
    if (next_recv_head != console_recv_tail)
    {
      // Insert the character into the receive buffer.
      console_recv_buffer[console_recv_head] = ch;

      // Update the receive head.
      console_recv_head = next_recv_head;
    }

    // Signal the receive event.
    osEventFlagsSet(console_event_id, CONSOLE_EVENT_RECV);

    // Notify the console thread of I/O available. This is a more general purpose
    // event specifically on the console thread to indicate I/O has taken place.
    if (console_thread_id) osThreadFlagsSet(console_thread_id, CONSOLE_THREAD_EVENT_IO);
  }
}
#endif

#if defined(USE_STM32F4_DISCOVERY)
void USART6_IRQHandler(void)
{
  // Check for the TXE interrupt.
  if (((LL_USART_IsEnabledIT_TXE(CONSOLE_USART)) && (LL_USART_IsActiveFlag_TXE(CONSOLE_USART))))
  {
    // Are there characters to send?
    if (console_send_tail != console_send_head)
    {
      // Put the next character in the transmit buffer which clears the interrupt.
      LL_USART_TransmitData9(CONSOLE_USART, (uint16_t) console_send_buffer[console_send_tail]);

      // Increment the tail.
      console_send_tail = (console_send_tail + 1) % CONSOLE_SEND_BUFFER_SIZE;
    }

    // Disable transmitter empty if all data is sent.
    if (console_send_tail == console_send_head)
    {
      // Disable the transmit buffer empty interrupt.
      LL_USART_DisableIT_TXE(CONSOLE_USART);
    }

    // Signal the send event.
    osEventFlagsSet(console_event_id, CONSOLE_EVENT_SEND);
  }

  // Is the USART_IT_RXNE interrupt enabled?
  if (((LL_USART_IsEnabledIT_RXNE(CONSOLE_USART)) && (LL_USART_IsActiveFlag_RXNE(CONSOLE_USART))))
  {
    // Read the received character which will clear the interrupt.
    char ch = (char) (LL_USART_ReceiveData9(CONSOLE_USART) & 0x7f);

    // Determine the next head index.
    size_t next_recv_head = (console_recv_head + 1) % CONSOLE_RECV_BUFFER_SIZE;

    // Make sure receiving this character doesn't overflow the receive buffer.
    if (next_recv_head != console_recv_tail)
    {
      // Insert the character into the receive buffer.
      console_recv_buffer[console_recv_head] = ch;

      // Update the receive head.
      console_recv_head = next_recv_head;
    }

    // Signal the receive event.
    osEventFlagsSet(console_event_id, CONSOLE_EVENT_RECV);

    // Notify the console thread of I/O available. This is a more general purpose
    // event specifically on the console thread to indicate I/O has taken place.
    if (console_thread_id) osThreadFlagsSet(console_thread_id, CONSOLE_THREAD_EVENT_IO);
  }
}
#endif

#if defined(USE_STM32F405_OLIMEX)
void USART6_IRQHandler(void)
{
  // Check for the TXE interrupt.
  if (((LL_USART_IsEnabledIT_TXE(CONSOLE_USART)) && (LL_USART_IsActiveFlag_TXE(CONSOLE_USART))))
  {
    // Are there characters to send?
    if (console_send_tail != console_send_head)
    {
      // Put the next character in the transmit buffer which clears the interrupt.
      LL_USART_TransmitData9(CONSOLE_USART, (uint16_t) console_send_buffer[console_send_tail]);

      // Increment the tail.
      console_send_tail = (console_send_tail + 1) % CONSOLE_SEND_BUFFER_SIZE;
    }

    // Disable transmitter empty if all data is sent.
    if (console_send_tail == console_send_head)
    {
      // Disable the transmit buffer empty interrupt.
      LL_USART_DisableIT_TXE(CONSOLE_USART);
    }

    // Signal the send event.
    osEventFlagsSet(console_event_id, CONSOLE_EVENT_SEND);
  }

  // Is the USART_IT_RXNE interrupt enabled?
  if (((LL_USART_IsEnabledIT_RXNE(CONSOLE_USART)) && (LL_USART_IsActiveFlag_RXNE(CONSOLE_USART))))
  {
    // Read the received character which will clear the interrupt.
    char ch = (char) (LL_USART_ReceiveData9(CONSOLE_USART) & 0x7f);

    // Determine the next head index.
    size_t next_recv_head = (console_recv_head + 1) % CONSOLE_RECV_BUFFER_SIZE;

    // Make sure receiving this character doesn't overflow the receive buffer.
    if (next_recv_head != console_recv_tail)
    {
      // Insert the character into the receive buffer.
      console_recv_buffer[console_recv_head] = ch;

      // Update the receive head.
      console_recv_head = next_recv_head;
    }

    // Signal the receive event.
    osEventFlagsSet(console_event_id, CONSOLE_EVENT_RECV);

    // Notify the console thread of I/O available. This is a more general purpose
    // event specifically on the console thread to indicate I/O has taken place.
    if (console_thread_id) osThreadFlagsSet(console_thread_id, CONSOLE_THREAD_EVENT_IO);
  }
}
#endif

// NT Shell read function.
static int console_ntshell_read(char *buffer, int buflen, void *extobj)
{
  (void) extobj;
  int i;

  // Get the indicated number of characters.
  for (i = 0; i < buflen; i++) buffer[i] = (char) console_getc();

  return buflen;
}

// NT Shell write function.
static int console_ntshell_write(const char *buffer, int buflen, void *extobj)
{
  (void) extobj;
  int i;

  // Put the indicated number of characters.
  for (i = 0; i < buflen; i++) console_putc(buffer[i]);

  return buflen;
}

// NT Shell application callback.
static int console_ntshell_callback(const char *text, void *extobj)
{
  (void) extobj;

  // Specify a static temporary buffer.
  static char buffer[NTCONF_EDITOR_MAXLEN];

  // Copy the text into the buffer.
  strcpy(buffer, text);

  // Execute the shell command. Return non-zero will exit the shell.
  return shell_command(buffer) ? 1 : 0;
}

// Thread that executes the console shell.
static void console_thread(void *arg)
{
  // Enable the next RXNE interrupt.
  LL_USART_EnableIT_RXNE(CONSOLE_USART);

  // Run the console shell.
  for (;;) 
  {
    // Tell the user the shell is starting.
    console_puts("Starting console shell...\n");

    // Initialize and execute the NT shell.
    static ntshell_t console_ntshell;
    ntshell_init(&console_ntshell, console_ntshell_read, console_ntshell_write, console_ntshell_callback, (void *) &console_ntshell);
    ntshell_set_prompt(&console_ntshell, "% ");
    ntshell_execute(&console_ntshell);
  }
}

// Initialize the console UART. Does not initialize console shell.
// WARNING: This function called prior to RTOS initialization.
void console_init(void)
{
  LL_GPIO_InitTypeDef gpio_init;
  LL_USART_InitTypeDef usart_init;

  // Enable peripheral clocks.
  LL_AHB1_GRP1_EnableClock(CONSOLE_PERIPH_GPIO);
  CONSOLE_PERIPH_ENABLECLK(CONSOLE_PERIPH_USART);

  // Configure GPIO pin for USART TX.
  gpio_init.Pin = CONSOLE_TX_PIN;
  gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.Pull = LL_GPIO_PULL_UP;
  gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
  gpio_init.Alternate = CONSOLE_GPIO_AF;
  LL_GPIO_Init(CONSOLE_GPIO, &gpio_init);

  // Configure GPIO pin for USART RX.
  gpio_init.Pin = CONSOLE_RX_PIN;
  gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.Pull = LL_GPIO_PULL_UP;
  gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
  gpio_init.Alternate = CONSOLE_GPIO_AF;
  LL_GPIO_Init(CONSOLE_GPIO, &gpio_init);

  // USARTx configured as follows: 
  // 115200 baud, 8 Bits, 1 Stop, No Parity, 
  // No Flow Control, Receive and Transmit Enabled.
  LL_USART_StructInit(&usart_init);
  usart_init.BaudRate = console_config_baudrate();
  usart_init.StopBits = console_config_stop_bits();
  usart_init.Parity = console_config_parity();
  LL_USART_Init(CONSOLE_USART, &usart_init);

#ifdef STM32F7
  // Disable overrun detection.
  LL_USART_DisableOverrunDetect(CONSOLE_USART);
#endif

  // Disable USART interrupts.
  LL_USART_DisableIT_CTS(CONSOLE_USART);
  LL_USART_DisableIT_LBD(CONSOLE_USART);
  LL_USART_DisableIT_TXE(CONSOLE_USART);
  LL_USART_DisableIT_TC(CONSOLE_USART);
  LL_USART_DisableIT_RXNE(CONSOLE_USART);
  LL_USART_DisableIT_IDLE(CONSOLE_USART);
  LL_USART_DisableIT_PE(CONSOLE_USART);
  LL_USART_DisableIT_ERROR(CONSOLE_USART);

  // Enable global interrupts for console USART.
  uint32_t grouping = NVIC_GetPriorityGrouping();
  uint32_t priority = NVIC_EncodePriority(grouping, console_config_preempt_priority(), 0);
  NVIC_SetPriority(CONSOLE_IRQ, priority);
  NVIC_EnableIRQ(CONSOLE_IRQ);

  // Enable the USART.
  LL_USART_Enable(CONSOLE_USART);
}

// De-initialize UART console.
void console_deinit(void)
{
  // Disable console IRQ.
  NVIC_DisableIRQ(CONSOLE_IRQ);

  // Disable the USART.
  LL_USART_Disable(CONSOLE_USART);

  // Restore UART to its default state.
  LL_USART_DeInit(CONSOLE_USART);
}

// Launch the shell on the console.
void console_shell_init(void)
{
  // Static mutex, event and thread control blocks.
  static uint32_t console_mutex_cb[osRtxMutexCbSize/4U] __attribute__((section(".bss.os.mutex.cb")));
  static uint32_t console_event_cb[osRtxEventFlagsCbSize/4U] __attribute__((section(".bss.os.evflags.cb")));
  static uint32_t console_thread_cb[osRtxThreadCbSize/4U] __attribute__((section(".bss.os.thread.cb")));

  // Console control event attributes.
  osEventFlagsAttr_t console_event_attrs =
  {
    .name = "console",
    .attr_bits = 0U,
    .cb_mem = console_event_cb,
    .cb_size = sizeof(console_event_cb)
  };

  // Console mutex attributes. Note the mutex is not recursive.
  osMutexAttr_t console_mutex_attrs =
  {
    .name = "console",
    .attr_bits = 0U,
    .cb_mem = console_mutex_cb,
    .cb_size = sizeof(console_mutex_cb)
  };

  // Console thread attributes.
  osThreadAttr_t console_thread_attrs =
  {
    .name = "console", .attr_bits = 0U,
    .cb_mem = console_thread_cb,
    .cb_size = sizeof(console_thread_cb),
    .priority = osPriorityBelowNormal
  };

  // Create the console event flags.
  console_event_id = osEventFlagsNew(&console_event_attrs);
  if (console_event_id == NULL)
  {
    syslog_printf(SYSLOG_ERROR, "CONSOLE: cannot create event flags");
  }

  // Create the console mutex.
  console_mutex_id = osMutexNew(&console_mutex_attrs);
  if (console_mutex_id == NULL)
  {
    syslog_printf(SYSLOG_ERROR, "CONSOLE: cannot create mutex");
  }

  // Initialize the console thread.
  console_thread_id = osThreadNew(console_thread, NULL, &console_thread_attrs);
  if (console_thread_id != NULL)
  {
    // Fill in the shell client structure.
    shell_client_t shell_client = { console_thread_id, console_hasc, console_getc, console_putc,
                                    console_flush, console_reboot, console_shutdown };

    // Register the console shell client.
    shell_add_client(&shell_client);
  }
  else
  {
    syslog_printf(SYSLOG_ERROR, "CONSOLE: cannot create thread");
  }
}

// Get the string from the console stream.
bool console_gets(char *buffer, int len, int tocase, bool echo)
{
  int c;
  int i;

  i = 0;
  for (;;)
  {
    // Get a char from the incoming stream.
    c = console_getc();

    // End of stream?
    if (c < 0) return false;

    // Convert to upper/lower case?
    if (tocase > 0) c = toupper(c);
    if (tocase < 0) c = tolower(c);

    // End of line?
    if (c == '\r') break;

    // Back space?
    if (((c == '\b') && i) || ((c == 0x7f) && i))
    {
      i--;
      if (echo) { console_putc(c); console_putc(' '); console_putc(c); }
    }

    // Visible chars.
    if ((c >= ' ') && (c < 0x7f) && (i < (len - 1)))
    {
      buffer[i++] = c;
      if (echo) console_putc(c);
    }
  }

  // Null terminate.
  buffer[i] = 0;
  if (echo) console_puts("\r\n");

  return true;
}

// Returns 1 if there is a next character to be read from the UART, 0 if
// not or -1 on end of stream or error.
int console_hasc(void)
{
  int retval = 0;

  // Are we in a thread?
  if ((osThreadGetId() == NULL) || (console_mutex_id == NULL))
  {
    // No. Do nothing.
  }
  else
  {
    // Disable the interrupts.  We do this to protect the head and tail
    // buffer indices from being changed by the interrupt handler.
    __disable_irq();

    // Is there UART data ready?
    retval = (console_recv_head == console_recv_tail) ? 0 : 1;

    // Enable interrupts.
    __enable_irq();
  }

  return retval;
}

// Returns the next character read from the UART as an unsigned char
// cast to an int or -1 on end of stream or error.
int console_getc(void)
{
  int retval = -1;

  // Are we in a thread?
  if ((osThreadGetId() == NULL) || (console_mutex_id == NULL))
  {
    // No. Do nothing.
  }
  else
  {
    // Clear the received data event flag. This will be set when new data is received.
    osEventFlagsClear(console_event_id, CONSOLE_EVENT_RECV);

    // Disable the interrupts.  We do this to protect the head and tail
    // buffer indices from being changed by the interrupt handler.
    __disable_irq();

    // Wait until there is data in the receive buffer.
    while (console_recv_head == console_recv_tail)
    {
      // Enable interrupts.
      __enable_irq();

      // Wait up to 500 milliseconds for a recv signal to occur.
      osEventFlagsWait(console_event_id, CONSOLE_EVENT_RECV, osFlagsWaitAny, 500);

      // Disable interrupts.
      __disable_irq();
    }

    // Get the character from the receive buffer.
    retval = console_recv_buffer[console_recv_tail];

    // Update the receive tail.
    console_recv_tail = (console_recv_tail + 1) % CONSOLE_RECV_BUFFER_SIZE;

    // Enable interrupts.
    __enable_irq();
  }

  return retval;
}

// Put a string out to the UART.
void console_puts(const char *str)
{
  // Send each character in the string with newline conversion.
  while (*str) { if (*str == '\n') console_putc('\r'); console_putc(*(str++)); }
}

// Output character to console stream.
void console_putc(int ch)
{
  // Are we in a thread?
  if ((osThreadGetId() == NULL) || (console_mutex_id == NULL))
  {
    // No. Wait until the USART transmit buffer is empty?
    while (LL_USART_IsActiveFlag_TXE(CONSOLE_USART) == RESET) { }

    // Send the character.
    LL_USART_TransmitData9(CONSOLE_USART, (uint16_t) ch);
  }
  else
  {
    // Lock the console mutex.
    osMutexAcquire(console_mutex_id, osWaitForever);

    // Disable interrupts. We do this to protect the head and tail
    // buffer indices from being changed by the interrupt handler.
    __disable_irq();

    // Get the next send head.
    size_t next_send_head = (console_send_head + 1) % CONSOLE_SEND_BUFFER_SIZE;

    // Is the send buffer filled?
    while (next_send_head == console_send_tail)
    {
      // Enable interrupts.
      __enable_irq();

      // Wait up to 500 milliseconds for a sent data event to occur.
      osEventFlagsWait(console_event_id, CONSOLE_EVENT_SEND, osFlagsWaitAny, 500);

      // Disable interrupts.
      __disable_irq();
    }

    // Insert the character into the send buffer.
    console_send_buffer[console_send_head] = (char) ch;

    // Update the send buffer head.
    console_send_head = next_send_head;

    // Enable the TXE interrupt.
    LL_USART_EnableIT_TXE(CONSOLE_USART);

    // Enable interrupts.
    __enable_irq();

    // Release the console mutex.
    osMutexRelease(console_mutex_id);
  }
}

// Output character to console stream from an interrupt context.
void console_putc_interrupt(int ch)
{
  // Send return character preceeding a each newline character.
  if (ch == '\n') console_putc_interrupt('\r');

  // No. Wait until the USART transmit buffer is empty?
  while (LL_USART_IsActiveFlag_TXE(CONSOLE_USART) == RESET) { }

  // Send the character.
  LL_USART_TransmitData9(CONSOLE_USART, (uint16_t) ch);
}

// Output the buffer to the consle stream.
void console_put_buf(char *buffer, int len)
{
  // Send each character in the buffer.
  for (int i = 0; i < len; ++i) console_putc(buffer[i]);
}

// Called to flush the output buffers.
void console_flush(void)
{
  // Does nothing for the console.
}

// Called to reboot from the console.
void console_reboot(void)
{
  // Immediately reboot the system.
  reboot_system();
}

// Called to shutdown from the console.
void console_shutdown(void)
{
  // Immediately shutdown the system.
  shutdown_system();
}

// System configurable console UART baudrate.
__WEAK uint32_t console_config_baudrate(void)
{
  return 115200u;
}

// System configurable console UART stop bits.
__WEAK uint32_t console_config_stop_bits(void)
{
  return LL_USART_STOPBITS_1;
}

// System configurable console UART parity.
__WEAK uint32_t console_config_parity(void)
{
  return LL_USART_PARITY_NONE;
}

// System configurable console preemptive interrupt priority value.
__WEAK uint32_t console_config_preempt_priority(void)
{
  return 6;
}

