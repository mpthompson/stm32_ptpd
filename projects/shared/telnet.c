#include <ctype.h>
#include <string.h>
#include "cmsis_os2.h"
#include "rtx_lib.h"
#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "shell.h"
#include "delay.h"
#include "event.h"
#include "tick.h"
#include "syslog.h"
#include "network.h"
#include "outputf.h"
#include "ntshell.h"
#include "ntlibc.h"
#include "telnet.h"

// Telnet signal values.
#define TELNET_EVENT_ACCEPT      0x0002U
#define TELNET_EVENT_RECV        0x0004U
#define TELNET_EVENT_SENT        0x0008U

// Telnet thread event flag values.
#define TELNET_THREAD_EVENT_IO   0x40000000u

// Telnet states
#define STATE_NORMAL                  0
#define STATE_IAC                     1
#define STATE_OPT                     2
#define STATE_SB                      3
#define STATE_OPTDAT                  4
#define STATE_SE                      5
#define STATE_EOL                     6

// Telnet special characters
#define TELNET_SE                   240   // End of subnegotiation parameters
#define TELNET_NOP                  241   // No operation
#define TELNET_MARK                 242   // Data mark
#define TELNET_BRK                  243   // Break
#define TELNET_IP                   244   // Interrupt process
#define TELNET_AO                   245   // Abort output
#define TELNET_AYT                  246   // Are you there
#define TELNET_EC                   247   // Erase character
#define TELNET_EL                   248   // Erase line
#define TELNET_GA                   249   // Go ahead
#define TELNET_SB                   250   // Start of subnegotiation parameters
#define TELNET_WILL                 251   // Will option code
#define TELNET_WONT                 252   // Won't option code
#define TELNET_DO                   253   // Do option code
#define TELNET_DONT                 254   // Don't option code
#define TELNET_IAC                  255   // Interpret as command

// Telnet options
#define TELOPT_TRANSMIT_BINARY        0   // Binary Transmission (RFC856)
#define TELOPT_ECHO                   1   // Echo (RFC857)
#define TELOPT_SUPPRESS_GO_AHEAD      3   // Suppress Go Ahead (RFC858)
#define TELOPT_STATUS                 5   // Status (RFC859)
#define TELOPT_TIMING_MARK            6   // Timing Mark (RFC860)
#define TELOPT_NAOCRD                10   // Output Carriage-Return Disposition (RFC652)
#define TELOPT_NAOHTS                11   // Output Horizontal Tab Stops (RFC653)
#define TELOPT_NAOHTD                12   // Output Horizontal Tab Stop Disposition (RFC654)
#define TELOPT_NAOFFD                13   // Output Formfeed Disposition (RFC655)
#define TELOPT_NAOVTS                14   // Output Vertical Tabstops (RFC656)
#define TELOPT_NAOVTD                15   // Output Vertical Tab Disposition (RFC657)
#define TELOPT_NAOLFD                16   // Output Linefeed Disposition (RFC658)
#define TELOPT_EXTEND_ASCII          17   // Extended ASCII (RFC698)
#define TELOPT_TERMINAL_TYPE         24   // Terminal Type (RFC1091)
#define TELOPT_NAWS                  31   // Negotiate About Window Size (RFC1073)
#define TELOPT_TERMINAL_SPEED        32   // Terminal Speed (RFC1079)
#define TELOPT_TOGGLE_FLOW_CONTROL   33   // Remote Flow Control (RFC1372)
#define TELOPT_LINEMODE              34   // Linemode (RFC1184)
#define TELOPT_AUTHENTICATION        37   // Authentication (RFC1416)

#define TERM_UNKNOWN                  0
#define TERM_CONSOLE                  1
#define TERM_VT100                    2

#define TELNET_BUF_SIZE             512

// Terminal characteristics.
typedef struct term
{
  int32_t type;
  int32_t cols;
  int32_t lines;
} term_t;

// Terminal options state.
typedef struct termstate
{
  int32_t state;
  int32_t code;
  uint8_t optdata[32];
  uint16_t optlen;
  term_t term;
} termstate_t;

// Telnet options.
static termstate_t tstate;

// Telnet event flags id.
static osEventFlagsId_t telnet_event_id = NULL;

// Telnet thread id.
static osThreadId_t telnet_thread_id = NULL;

// Telnet timer id.
static osTimerId_t telnet_timer_id = NULL;

// Telnet connection control block.
static struct tcp_pcb *telnet_client_pcb = NULL;

// Telnet receive state.
static osMutexId_t telnet_recv_mutex_id = NULL;
static struct pbuf *telnet_recv_chain = NULL;
static volatile bool telnet_recv_eof = true;

// Telnet flags at end of stream.
static volatile bool reboot_on_exit = false;
static volatile bool shutdown_on_exit = false;

// Telnet buffers.
static uint8_t telnet_recv_buffer[128];
static uint8_t telnet_send_buffer[128];
static volatile int telnet_recv_index = 0;
static volatile int telnet_recv_count = 0;
static volatile int telnet_send_tail = 0;
static volatile int telnet_send_sent = 0;
static volatile int telnet_send_head = 0;

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

// Send the specified telnet option.
static void telnet_sendopt(termstate_t *ts, int code, int option)
{
  // Send the option and flush.
  telnet_putc(TELNET_IAC);
  telnet_putc(code);
  telnet_putc(option);
}

// Parse the options and send a response in certain cases.
static void telnet_parseopt(termstate_t *ts, int code, int option)
{
  switch (option)
  {
    case TELOPT_ECHO:
      break;
    case TELOPT_SUPPRESS_GO_AHEAD:
      if (code == TELNET_WILL || code == TELNET_WONT)
        telnet_sendopt(ts, TELNET_DO, option);
      else
        telnet_sendopt(ts, TELNET_WILL, option);
      break;
    case TELOPT_TERMINAL_TYPE:
    case TELOPT_NAWS:
    case TELOPT_TERMINAL_SPEED:
      telnet_sendopt(ts, TELNET_DO, option);
      break;
    default:
      if ((code == TELNET_WILL) || (code == TELNET_WONT))
        telnet_sendopt(ts, TELNET_DONT, option);
      else
        telnet_sendopt(ts, TELNET_WONT, option);
      break;
  }
}

// Parse the option data.
static void telnet_parseoptdat(termstate_t *ts, int32_t option, uint8_t *data, uint16_t len)
{
  switch (option)
  {
    case TELOPT_NAWS:
      if (len == 4)
      {
        int32_t cols = lwip_ntohs(*(uint16_t *) data);
        int32_t lines = lwip_ntohs(*(uint16_t *) (data + 2));
        if (cols) ts->term.cols = cols;
        if (lines) ts->term.lines = lines;
      }
      break;
    case TELOPT_TERMINAL_SPEED:
      break;
    case TELOPT_TERMINAL_TYPE:
      break;
  }
}

// Process the buffer looking for telnet options passed from the telnet client.
// Returns the size of the buffer after processing.
static int telnet_process(termstate_t *ts, uint8_t *buffer, int buflen)
{
  uint8_t *pbuf = buffer;
  uint8_t *pend = pbuf + buflen;
  uint8_t *qbuf = pbuf;
  while (pbuf < pend)
  {
    int32_t c = (int32_t) *pbuf++;
    switch (ts->state) 
    {
      case STATE_NORMAL:
        switch (c)
        {
          case TELNET_IAC:
            ts->state = STATE_IAC;
            break;
          case '\r':
            *qbuf++ = c;
            ts->state = STATE_EOL;
            break;
          default:
           *qbuf++ = c;
        }
        break;
      case STATE_IAC:
        switch (c) 
        {
          case TELNET_IAC:
            *qbuf++ = c;
            ts->state = STATE_NORMAL;
            break;
          case TELNET_WILL:
          case TELNET_WONT:
          case TELNET_DO:
          case TELNET_DONT:
            ts->code = c;
            ts->state = STATE_OPT;
            break;
          case TELNET_SB:
            ts->state = STATE_SB;
            break;
          default:
            ts->state = STATE_NORMAL;
        }
        break;
      case STATE_OPT:
        telnet_parseopt(ts, ts->code, c);
        ts->state = STATE_NORMAL;
        break;
      case STATE_SB:
        ts->code = c;
        ts->optlen = 0;
        ts->state = STATE_OPTDAT;
        break;
      case STATE_OPTDAT:
        if (c == TELNET_IAC)
          ts->state = STATE_SE;
        else if (ts->optlen < sizeof(ts->optdata))
          ts->optdata[ts->optlen++] = c;
        break;
      case STATE_SE:
        if (c == TELNET_SE)
          telnet_parseoptdat(ts, ts->code, ts->optdata, ts->optlen);
        ts->state = STATE_NORMAL;
        break;
      case STATE_EOL:
        // Special state to deal with Telnet End-of-Line convention.
        // See: RFC 1123, Section: 3.3.1
        // Here we want to convert sequence '\r' + '\n' to '\r' only.
        if (c != '\n')
          *qbuf++ = c;
        ts->state = STATE_NORMAL;
        break;
    } 
  }
  return (int) (qbuf - buffer);
}

// NT Shell read function.
static int telnet_ntshell_read(char *buffer, int buflen, void *extobj)
{
  (void) extobj;
  int i;

  // Get the indicated number of characters.
  for (i = 0; i < buflen; ++i)
  {
    // Get the next character.
    int ch = telnet_getc();

    // Look for errors or end of stream.
    if (ch < 0) return -1;

    // Insert the character into the buffer.
    buffer[i] = (char) ch;
  }

  return buflen;
}

// NT Shell write function.
static int telnet_ntshell_write(const char *buffer, int buflen, void *extobj)
{
  (void) extobj;
  int i;

  // Write and flush the telnet buffer.
  for (i = 0; i < buflen; ++i)
  {
    // Output the character with the last character flushing the stream.
    telnet_putc(buffer[i]);
  }

  return buflen;
}

// NT Shell application callback.
static int telnet_ntshell_callback(const char *text, void *extobj)
{
  (void) extobj;

  // Specify a static temporary buffer.
  static char buffer[NTCONF_EDITOR_MAXLEN];

  // Copy the text into the buffer.
  strcpy(buffer, text);

  // Execute the shell command. Return non-zero will exit the shell.
  return !shell_command(buffer) && !telnet_recv_eof ? 0 : 1;
}

// Handles closing the connection cleanup.
static void telnet_reset_client(void)
{
  // Was there a client connection?
  if (telnet_client_pcb != NULL)
  {
    // Set the eof flag.
    telnet_recv_eof = true;

    // No more client connection.
    telnet_client_pcb = NULL;
  }
}

// Handles closing the connection cleanup.
static void telnet_close_client(void)
{
  // Lock the lwIP core mutex.
  LOCK_TCPIP_CORE();

  // Was there a client connection?
  if (telnet_client_pcb != NULL)
  {
    // Close the connection.
    tcp_close(telnet_client_pcb);

    // Clean up the connection.
    telnet_reset_client();
  }

  // Unlock the lwIP core mutex.
  UNLOCK_TCPIP_CORE();
}

// Get data from the receive buffers. Buflen must be non-zero.
static int telnet_recv_data(void *buffer, uint16_t buflen, bool block)
{
  // Initialize buffer pointers for data.
  uint8_t *pbuf = (uint8_t *) buffer;
  uint8_t *pend = pbuf + buflen;

  // Lock the telnet mutex.
  osMutexAcquire(telnet_recv_mutex_id, osWaitForever);

  // Block while there is no data to be read.
  while (block && !telnet_recv_eof && (telnet_recv_chain == NULL))
  {
    // Release the telnet receive mutex.
    osMutexRelease(telnet_recv_mutex_id);

    // Wait up to 1000 milliseconds before trying to see if more data is available.
    osEventFlagsWait(telnet_event_id, TELNET_EVENT_RECV, osFlagsWaitAny, 1000);

    // Lock the telnet receive mutex.
    osMutexAcquire(telnet_recv_mutex_id, osWaitForever);
  }

  // Read as much data as possible from the receive buffer.
  while (telnet_recv_chain && (pbuf < pend))
  {
    // Get the payload data pointer.
    uint8_t *data = (uint8_t *) telnet_recv_chain->payload;

    // Copy data from the payload.
    while (telnet_recv_chain->len && (pbuf < pend))
    {
      // Copy the next character.
      *(pbuf++) = *(data++);

      // Decrement the data left in the payload.
      --telnet_recv_chain->len;
      --telnet_recv_chain->tot_len;
    }

    // Update the payload data pointer.
    telnet_recv_chain->payload = data;

    // Save the free buffers.
    if (!telnet_recv_chain->len)
    {
      // Pull the buffer from our chain of buffers.
      struct pbuf *free_buffer = telnet_recv_chain;
      telnet_recv_chain = free_buffer->next;
      free_buffer->next = NULL;
      free_buffer->tot_len = 0;
      pbuf_free(free_buffer);
    }
  }

  // Determine how much data was written.
  int count = (int) (pbuf - ((uint8_t *) buffer));

  // Release the telnet receive mutex.
  osMutexRelease(telnet_recv_mutex_id);

  // Did we succeed in reading data?
  if (count)
  {
    // Lock the lwIP core mutex.
    LOCK_TCPIP_CORE();

    // Inform TCP client of the amount of processed data.
    tcp_recved(telnet_client_pcb, count);

    // Unlock the lwIP core mutex.
    UNLOCK_TCPIP_CORE();
  }

  // Return -1 if at end of telnet stream and no more data.
  return count > 0 ? count : (telnet_recv_eof ? -1 : 0);
}

// Discard data from the receive buffers.
static uint16_t telnet_recv_discard(void)
{
  uint16_t tot_len = 0;

  // Lock the telnet mutex.
  osMutexAcquire(telnet_recv_mutex_id, osWaitForever);

  // Discard an received data.
  if (telnet_recv_chain)
  {
    // Get the total length of data to discard.
    tot_len = telnet_recv_chain->tot_len;

    // Free the buffered data.
    pbuf_free(telnet_recv_chain);

    // Reset the chain.
    telnet_recv_chain = NULL;
  }

  // Release the telnet receive mutex.
  osMutexRelease(telnet_recv_mutex_id);

  // Was there data discarded?
  if (tot_len)
  {
    // Lock the lwIP core mutex.
    LOCK_TCPIP_CORE();

    // Inform TCP client of the amount of processed data.
    tcp_recved(telnet_client_pcb, tot_len);

    // Unlock the lwIP core mutex.
    UNLOCK_TCPIP_CORE();
  }

  return tot_len;
}

// Received data callback from the context of the tcpip thread.
static err_t telnet_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *buffer, err_t err)
{
  (void) arg;

  // Are we receiving data?
  if (buffer != NULL)
  {
    // Lock the telnet mutex.
    osMutexAcquire(telnet_recv_mutex_id, osWaitForever);

    // Concat the incoming packet buffer to the existing chain of packet buffers.
    // This takes over the current references to the incoming packet buffers.
    if (telnet_recv_chain)
      pbuf_cat(telnet_recv_chain, buffer);
    else
      telnet_recv_chain = buffer;

    // Release the telnet mutex.
    osMutexRelease(telnet_recv_mutex_id);
  }
  else if (err == ERR_OK)
  {
    // No data arrived. That means the client closes the connection and
    // sent us a packet with FIN flag set to 1. We must cleanup and
    // close our TCP connection.
    telnet_close_client();
  }

  // Set the event flag to indicate new data has been received.
  osEventFlagsSet(telnet_event_id, TELNET_EVENT_RECV);

  // Notify the telnet thread of I/O available. This is a more general purpose
  // event specifically on the telnet thread to indicate I/O has taken place.
  if (telnet_thread_id) osThreadFlagsSet(telnet_thread_id, TELNET_THREAD_EVENT_IO);

  // We succeeded.
  return ERR_OK;
}

// Sent callback from the context of the tcpip thread. This tells us how
// much of the data that was sent was acknowledged by the far end of the
// socket which for us increments the tail of the send buffer.  We should
// never get more data acknowledge than was actually sent.
static err_t telnet_sent_callback(void *arg, struct tcp_pcb *pcb, u16_t len)
{
  // Is the client connection active?
  if (telnet_client_pcb)
  {
    // Increment the at the tail of the buffer.
    telnet_send_tail = (telnet_send_tail + (int) len) % (int) sizeof(telnet_send_buffer);

    // Signal any waiting telnet thread that data in the buffer is now available.
    osEventFlagsSet(telnet_event_id, TELNET_EVENT_SENT);
  }

  // We succeeded.
  return ERR_OK;
}

// Error callback from the context of the tcpip thread. IMPORTANT: The
// corresponding pcb is already freed when this callback is called!
static void telnet_error_callback(void *arg, err_t err)
{
  // Reset the client connection.
  telnet_reset_client();

  // Set the receive and sent flags in case a thread is waiting for them.
  osEventFlagsSet(telnet_event_id, TELNET_EVENT_RECV);
  osEventFlagsSet(telnet_event_id, TELNET_EVENT_SENT);
}

// Telnet server accept callback from the context of the tcpip thread.
static err_t telnet_accept_callback(void *arg, struct tcp_pcb *npcb, err_t err)
{
  (void)(arg);

  // Before starting a new connection, make sure we have finished processing
  // data from a previous session and we don't have an existing session.
  if (telnet_recv_eof && (telnet_client_pcb == NULL))
  {
    // Save the incoming connection.
    telnet_client_pcb = npcb;

#if 0
    // XXX Disabling the nagle alorithm here seems to cause an occasional hang-up
    // XXX in the functon telnet_recv_data() waiting for a pbuf to arrive. This
    // XXX needs to be investigated further as it seems the nagle algorithm should
    // XXX be disabled in a telnet server.
    // Disable nagle on this connection.
    tcp_nagle_disable(telnet_client_pcb);
#endif

    // Turn on TCP Keepalive for the given pcb.
    telnet_client_pcb->so_options |= SOF_KEEPALIVE;
    telnet_client_pcb->keep_cnt = 4;
    telnet_client_pcb->keep_idle = 30000;
    telnet_client_pcb->keep_intvl = 5000;

    // Specify the local argument for this connection.
    tcp_arg(telnet_client_pcb, NULL);

    // Subscribe an error callback function.
    tcp_err(telnet_client_pcb, &telnet_error_callback);

    // Subscribe a receive data callback function.
    tcp_recv(telnet_client_pcb, &telnet_recv_callback);

    // Subscribe a sent data callback function.
    tcp_sent(telnet_client_pcb, &telnet_sent_callback);

    // Reset the eof flag.
    telnet_recv_eof = false;

    // Signal the telnet thread that a connection has been accepted.
    osEventFlagsSet(telnet_event_id, TELNET_EVENT_ACCEPT);
  }
  else
  {
    // We only support a single client connection.
    tcp_close(npcb);
  }

  // We succeeded.
  return ERR_OK;
}

// Telnet server initialize callback from the context of the tcpip thread.
static void telnet_init_callback(void *arg)
{
  struct tcp_pcb *pcb;

  // Get the telnet server port.
  uint16_t port = telnet_config_port();

  // Bind a function to a tcp port.
  if ((pcb = tcp_new()) != NULL)
  {
    // Bind to the TCP port.
    if (tcp_bind(pcb, IP_ADDR_ANY, port) == ERR_OK)
    {
      // Create a listing endpoint.
      if ((pcb = tcp_listen(pcb)) != NULL)
      {
        // Set accept callback.
        tcp_accept(pcb, &telnet_accept_callback);
      }
      else
      {
        syslog_printf(SYSLOG_ERROR, "TELNET: unable to listen to TCP port");
      }
    }
    else
    {
      syslog_printf(SYSLOG_ERROR, "TELNET: unable to bind TCP port");
    }
  }
  else
  {
    syslog_printf(SYSLOG_ERROR, "TELNET: unable to create new TCP port");
  }
}

// This is the main thread that implements the telnet shell.
static void telnet_thread(void *arg)
{
  (void) arg;

  // Delay a few seconds to allow other modules to initialize.
  delay_ms(3000);

  // Clear telnet related event flags.
  osEventFlagsClear(telnet_event_id, TELNET_EVENT_ACCEPT | TELNET_EVENT_RECV | TELNET_EVENT_SENT);

  // Loop forever.
  for (;;)
  {
    // Reset the terminal state.
    memset(&tstate, 0, sizeof(tstate));
    tstate.state = STATE_NORMAL;
    tstate.term.type = TERM_VT100;
    tstate.term.cols = 80;
    tstate.term.lines = 25;

    // Initialize the reboot and shutdown on exit.
    reboot_on_exit = false;
    shutdown_on_exit = false;

    // Lock the lwIP core mutex.
    LOCK_TCPIP_CORE();

    // Reset the send and receive buffer. These need to be reset in sync with
    // each other so we use the lwIP core mutex to assure this.
    telnet_recv_index = 0;
    telnet_recv_count = 0;
    telnet_send_tail = 0;
    telnet_send_sent = 0;
    telnet_send_head = 0;

    // Unlock the lwIP core mutex.
    UNLOCK_TCPIP_CORE();

    // Notify status of telnet.
    syslog_printf(SYSLOG_INFO, "TELNET: waiting for connection");

    // Clear telnet receive and sent event flags.
    osEventFlagsClear(telnet_event_id, TELNET_EVENT_RECV | TELNET_EVENT_SENT);

    // Wait for an incoming connection to be accepted.
    osEventFlagsWait(telnet_event_id, TELNET_EVENT_ACCEPT, osFlagsWaitAny, osWaitForever);

    // Notify status of telnet.
    syslog_printf(SYSLOG_INFO, "TELNET: connection starting");

    // Send initial option that we'll echo on this side.
    telnet_sendopt(&tstate, TELNET_WILL, TELOPT_ECHO);

    // Tell the user the shell is starting.
    telnet_puts(network_get_hostname());
    telnet_puts(" ");
    telnet_puts(telnet_config_welcome());
    telnet_puts("\nStarting shell...\n");

    // Initialize and execute the NT shell.
    static ntshell_t telnet_ntshell;
    ntshell_init(&telnet_ntshell, telnet_ntshell_read, telnet_ntshell_write, telnet_ntshell_callback, (void *) &telnet_ntshell);
    ntshell_set_prompt(&telnet_ntshell, "> ");
    ntshell_execute(&telnet_ntshell);

    // Make sure the client connection is closed.
    telnet_close_client();

    // Discard any outstanding data.
    telnet_recv_discard();

    // Notify status of telnet.
    syslog_printf(SYSLOG_INFO, "TELNET: connection ended");

    // Should we reboot?
    if (reboot_on_exit)
    {
      // Reboot the system.
      reboot_system();
    }

    // Should we shutdown?
    if (shutdown_on_exit)
    {
      // Shutdown the system.
      shutdown_system();
    }
  }
}

// Telnet event handler called from the main thread.
static void telnet_event_handler(void *context)
{
  UNUSED(context);

  // Flush the telnet output buffers.
  telnet_flush();
}

// Telnet timer handler called from the timer thread with limited resources.
static void telnet_timer_handler(void *context)
{
  // Trigger an event on the main thread to flush the telnet output buffers.
  event_schedule(telnet_event_handler, context, 0);
}

// Initialize the telnet server.
void telnet_init(void)
{
  static uint32_t telnet_mutex_cb[osRtxMutexCbSize/4U] __attribute__((section(".bss.os.mutex.cb")));
  static uint32_t telnet_timer_cb[osRtxTimerCbSize/4U] __attribute__((section(".bss.os.timer.cb")));
  static uint32_t telnet_event_cb[osRtxEventFlagsCbSize/4U] __attribute__((section(".bss.os.evflags.cb")));
  static uint32_t telnet_thread_cb[osRtxThreadCbSize/4U] __attribute__((section(".bss.os.thread.cb")));

  // Telnet event attributes.
  osEventFlagsAttr_t event_attrs =
  {
    .name = "telnet",
    .attr_bits = 0,
    .cb_mem = telnet_event_cb,
    .cb_size = sizeof(telnet_event_cb)
  };

  // Telnet timer attributes.
  osTimerAttr_t timer_attrs =
  {
    .name = "telnet",
    .attr_bits = 0U,
    .cb_mem = telnet_timer_cb,
    .cb_size = sizeof(telnet_timer_cb)
  };

  // Telnet mutex attributes. Note the mutex is recursive.
  osMutexAttr_t mutex_attrs =
  {
    .name = "telnet",
    .attr_bits = osMutexRecursive,
    .cb_mem = telnet_mutex_cb,
    .cb_size = sizeof(telnet_mutex_cb)
  };

  // Telnet thread attributes.
  osThreadAttr_t telnet_thread_attrs =
  {
    .name = "telnet",
    .attr_bits = 0U,
    .cb_mem = telnet_thread_cb,
    .cb_size = sizeof(telnet_thread_cb),
    .priority = osPriorityNormal
  };

  // Create the telnet event flags.
  telnet_event_id = osEventFlagsNew(&event_attrs);
  if (telnet_event_id == NULL)
  {
    // Report an error.
    syslog_printf(SYSLOG_ERROR, "TELNET: cannot create event flags");
    return;
  }

  // Create the telnet timer.
  telnet_timer_id = osTimerNew(telnet_timer_handler, osTimerOnce, NULL, &timer_attrs);
  if (telnet_timer_id == NULL)
  {
    // Log the critical error.
    syslog_printf(SYSLOG_CRITICAL, "TELNET: error creating timer");
    return;
  }

  // Create the telnet mutex.
  telnet_recv_mutex_id = osMutexNew(&mutex_attrs);
  if (telnet_recv_mutex_id == NULL)
  {
    // Report an error.
    syslog_printf(SYSLOG_ERROR, "TELNET: cannot create mutex");
    return;
  }

  // Create the telnet processing thread.
  telnet_thread_id = osThreadNew(telnet_thread, NULL, &telnet_thread_attrs);
  if (telnet_thread_id != NULL)
  {
    // Fill in the shell client structure.
    shell_client_t shell_client = { telnet_thread_id, telnet_hasc, telnet_getc, telnet_putc,
                                    telnet_flush, telnet_reboot_on_exit, telnet_shutdown_on_exit };

    // Register the telnet shell client.
    shell_add_client(&shell_client);
  }
  else
  {
    // Report an error.
    syslog_printf(SYSLOG_ERROR, "TELNET: cannot create thread");
    return;
  }

  // Other initialization must occur within the context of the tcpip thread.
  tcpip_callback(telnet_init_callback, NULL);
}

// Flush whatever data is in the send buffer.
void telnet_flush(void)
{
  int index;
  int count;

  // Lock the lwIP core mutex.
  LOCK_TCPIP_CORE();

  // Make sure we have the pcb still.
  if (telnet_client_pcb)
  {
    // Set the index and count of data to send.
    index = telnet_send_sent;
    count = telnet_send_head >= telnet_send_sent ?
            telnet_send_head - telnet_send_sent :
            (int) sizeof(telnet_send_buffer) - telnet_send_sent;

    // Send whatever unsent data there is in the buffer.
    if (count && (tcp_write(telnet_client_pcb, &telnet_send_buffer[index], count, 0) == ERR_OK))
    {
      // Increment the sent index.
      telnet_send_sent = (telnet_send_sent + count) % (int) sizeof(telnet_send_buffer);

      // Send again if discontiguous.
      if (telnet_send_sent < telnet_send_head)
      {
        index = telnet_send_sent;
        count = telnet_send_head - telnet_send_sent;

        // Send what we can from the buffer.
        if (count && (tcp_write(telnet_client_pcb, &telnet_send_buffer[index], count, 0) == ERR_OK))
        {
          // Increment the sent index.
          telnet_send_sent = (telnet_send_sent + count) % (int) sizeof(telnet_send_buffer);
        }
      }

      // Send the data that was written above.
      tcp_output(telnet_client_pcb);
    }
  }

  // Unlock the lwIP core mutex.
  UNLOCK_TCPIP_CORE();
}

// Send a single character through the send buffer. This function may
// block if the buffer is filled and data has not yet been acknowledged
// by the far end of the socket.
void telnet_putc(int ch)
{
  // Lock the lwIP core mutex.
  LOCK_TCPIP_CORE();

  // Determine the next head index.
  int next_head = (telnet_send_head + 1) % (int) sizeof(telnet_send_buffer);

  // Is our buffer filled? If so, we need to wait until some data is
  // acknowledged by the far end of the socket before we can write new
  // data to the buffer.
  while (telnet_client_pcb && (next_head == telnet_send_tail))
  {
    // Send whatever unsent data there is in the buffer.
    telnet_flush();

    // Unlock the lwIP core mutex.
    UNLOCK_TCPIP_CORE();

    // Wait up to one second for data to be acknowledged by the far end of the
    // socket before we try again to test if space is available in the buffer.
    osEventFlagsWait(telnet_event_id, TELNET_EVENT_SENT, osFlagsWaitAny, 1000);

    // Lock the lwIP core mutex.
    LOCK_TCPIP_CORE();
  }

  // Make sure we have the pcb still.
  if (telnet_client_pcb)
  {
    // Insert the character into the buffer.
    telnet_send_buffer[telnet_send_head] = ch;

    // Update the send head.
    telnet_send_head = next_head;

    // Write the buffer if it is filled.
    if (telnet_send_head == telnet_send_tail)
    {
      // Send whatever unsent data there is in the buffer.
      telnet_flush();
    }
  }

  // Set the one-shot timer to automatically flush after 20 milliseconds.
  osTimerStart(telnet_timer_id, tick_from_milliseconds(20));

  // Unlock the lwIP core mutex.
  UNLOCK_TCPIP_CORE();
}

// Output the string to the telnet stream.
void telnet_puts(const char *str)
{
  // Write each character in the buffer to the telnet stream with newline conversion.
  while (*str) { if (*str == '\n') telnet_putc('\r'); telnet_putc(*(str++)); }
}

// Output formatted data to the telnet stream.
void telnet_printf(const char *fmt, ...)
{
  va_list arp;

  // Format the data passing output to telnet_putc().
  va_start(arp, fmt);
  vfoutputf(telnet_putc, fmt, arp);
  va_end(arp);
}

// Returns 1 if there is a next character to be read from the telnet, 0 if
// not or -1 on end of telnet stream or error.
int telnet_hasc(void)
{
  // Is there data in the receive buffer empty? The processing here is a little
  // convoluted as we need to make sure that there is no pending data in the
  // incoming receive buffers as well as all telnet options are processed from
  // any incoming data before finally determining there is no data to process.
  while (telnet_recv_index == telnet_recv_count)
  {
    // Reset the receive index.
    telnet_recv_index = 0;

    // Read data from the telnet stream without blocking.
    telnet_recv_count = telnet_recv_data(telnet_recv_buffer, sizeof(telnet_recv_buffer), false);

    // Did we hit an error?
    if (telnet_recv_count < 0) return -1;

    // Break if there is no data to process.
    if (telnet_recv_count < 1) break;
    
    // Process telnet options in the incoming buffer.
    telnet_recv_count = telnet_process(&tstate, telnet_recv_buffer, telnet_recv_count);

    // Did we hit an error?
    if (telnet_recv_count < 0) return -1;
  }

  // Return 1 if there are characters to process, 0 if not.
  return (telnet_recv_index == telnet_recv_count) ? 0 : 1;
}

// Returns the next character read from the telnet stream as an unsigned char
// cast to an int or -1 on end of telnet stream or error.
int telnet_getc(void)
{
  // Is there data in the receive buffer empty?
  while (telnet_recv_index == telnet_recv_count)
  {
    // Reset the receive index.
    telnet_recv_index = 0;

    // Read data from the telnet stream.
    telnet_recv_count = telnet_recv_data(telnet_recv_buffer, sizeof(telnet_recv_buffer), true);

    // Did we hit an error or reach the end of the telnet stream?
    if (telnet_recv_count < 1) return -1;

    // Process telnet options in the incoming buffer.
    telnet_recv_count = telnet_process(&tstate, telnet_recv_buffer, telnet_recv_count);

    // Did we hit an error?
    if (telnet_recv_count < 0) return -1;
  }

  // Return the next character from the receive buffer.
  return (int) telnet_recv_buffer[telnet_recv_index++];
}

// Get the string from the telnet stream.
bool telnet_gets(char *buffer, int len, int tocase, bool echo)
{
  int i;
  int ch;

  i = 0;
  for (;;)
  {
    // Get a char from the incoming stream.
    ch = telnet_getc();

    // End of stream?
    if (ch < 0) return false;

    // Convert to upper/lower case?
    if (tocase > 0) ch = toupper(ch);
    if (tocase < 0) ch = tolower(ch);

    // End of line?
    if (ch == '\r') break;

    // Back space?
    if ((i > 0) && ((ch == '\b') || (ch == 0x7f)))
    {
      // Decrement the index.
      i--;

      // If echoing erase the character.
      if (echo) { telnet_putc(ch); telnet_putc(' '); telnet_putc(ch); }
    }

    // Gather only visible characters.
    if ((ch >= ' ') && (ch < 0x7f) && (i < (len - 1)))
    {
      // Insert the character in the buffer.
      buffer[i++] = ch;

      // Are we echoing?
      if (echo) telnet_putc(ch);
    }

    // Flush buffers if echoing.
    if (echo) telnet_flush();
  }

  // Null terminate.
  buffer[i] = 0;

  // Newline if echoing.
  if (echo) telnet_puts("\n");

  return true;
}

// Returns true if the current thread is the telnet thread, otherwise false.
bool telnet_is_thread(void)
{
  return osThreadGetId() == telnet_thread_id ? true : false;
}

// Signals telnet to reboot the system on exit.
void telnet_reboot_on_exit(void)
{
  // Set the reboot on exit flag.
  reboot_on_exit = true;
}

// Signals telnet to shutdown the system on exit.
void telnet_shutdown_on_exit(void)
{
  // Set the shutdown on exit flag.
  shutdown_on_exit = true;
}

// System configurable telnet port.
__WEAK uint16_t telnet_config_port(void)
{
  return 23;
}

// System configurable telnet welcome string.
__WEAK const char *telnet_config_welcome(void)
{
  return "Embedded Telnet Server";
}

