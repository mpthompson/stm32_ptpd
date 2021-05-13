#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "cmsis_os2.h"
#include "rtx_os.h"
#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "shell.h"
#include "outputf.h"
#include "syslog.h"
#include "console.h"
#include "systime.h"

// Set the following to 1 to enable network syslog.
#if !defined(USE_NETWORK_SYSLOG)
#define USE_NETWORK_SYSLOG      0
#endif

// Size of the syslog buffer.
#define SYSLOG_SEND_SIZE              192
#define SYSLOG_BUFFER_SIZE            4096

// Syslog initialized flag.
static bool syslog_initialized = false;

// Syslog console echo flag.
static bool syslog_console_echo = true;

// Syslog level value.
static int syslog_level = SYSLOG_NOTICE;

// Syslog UDP control block.
static struct udp_pcb *syslog_pcb = NULL;

// Send buffer used to send a syslog message.
static uint32_t syslog_send_len = 0;
static char syslog_send_buffer[SYSLOG_SEND_SIZE];

// Buffered log structures.
static uint32_t syslog_buffer_head = 0;
static uint32_t syslog_buffer_tail = 0;
static uint32_t syslog_buffer_dump = 0;
static char syslog_buffer[SYSLOG_BUFFER_SIZE];

// Strings that store log levels.
static const char *syslog_levels[8] =
{
  "emergency",
  "alert",
  "critical",
  "error",
  "warning",
  "notice",
  "info",
  "debug"
};

// Mutex protection.
osMutexId_t syslog_mutex_id = NULL;

// Get the next character from the syslog buffer.
static char syslog_getc(void)
{
  char ch;

  // If there are no characters, return end-of-string character.
  if (syslog_buffer_tail == syslog_buffer_head) return 0;

  // Get the character to return.
  ch = syslog_buffer[syslog_buffer_tail];

  // Increment the tail.
  syslog_buffer_tail += 1;

  // Keep within the bounds of the buffer.
  if (syslog_buffer_tail >= SYSLOG_BUFFER_SIZE) syslog_buffer_tail = 0;

  return ch;
}

// Return if the syslog buffer is empty or not.
static bool syslog_buffer_empty(void)
{
  return syslog_buffer_tail == syslog_buffer_head ? true : false;
}

// Fill the send buffer with the next message.
static int syslog_next_message(void)
{
  char ch;

  // Reset the send length.
  syslog_send_len = 0;

  // Get the next character from the syslog buffer.
  ch = syslog_getc();

  // Insert the character into the send buffer.
  syslog_send_buffer[syslog_send_len] = ch;

  // Increment the send length.
  syslog_send_len += 1;

  // Keep going until we have extracted the next message.
  while (ch != 0)
  {
    // Get the next character from the syslog buffer.
    ch = syslog_getc();

    // Prevent overflowing the send buffer.
    if (syslog_send_len < SYSLOG_SEND_SIZE)
    {
      // Insert the character into the send buffer.
      syslog_send_buffer[syslog_send_len] = ch;

      // Increment the send length.
      syslog_send_len += 1;
    }
  }

  // Make sure the send buffer is always terminated by an end-of-string character.
  syslog_send_buffer[syslog_send_len - 1] = 0;

  // Return the length of the send buffer minus the end-of-string termination.
  return (int) syslog_send_len - 1;
}

// Put a character into the syslog.
static void syslog_putc(int ch)
{
  // Place the character at the head of the buffer.
  syslog_buffer[syslog_buffer_head] = (char) ch;

  // Increment the head within the bounds of the buffer.
  syslog_buffer_head = (syslog_buffer_head + 1) % SYSLOG_BUFFER_SIZE;

  // Have we collided with the tail indicating the buffer is full.
  if (syslog_buffer_head == syslog_buffer_tail)
  {
    // Increment the tail within the bounds of the buffer.
    syslog_buffer_tail = (syslog_buffer_tail + 1) % SYSLOG_BUFFER_SIZE;

    // Keep going until the entire message is dropped.
    while ((syslog_buffer[syslog_buffer_tail] != 0) && (syslog_buffer_tail != syslog_buffer_head))
    {
      // Increment the tail within the bounds of the buffer.
      syslog_buffer_tail = (syslog_buffer_tail + 1) % SYSLOG_BUFFER_SIZE;
    }
  }

  // Have we collided with the dump tail indicating the buffer is full.
  if (syslog_buffer_head == syslog_buffer_dump)
  {
    // Increment the dump tail within the bounds of the buffer.
    syslog_buffer_dump = (syslog_buffer_dump + 1) % SYSLOG_BUFFER_SIZE;

    // Keep going until the entire message is dropped.
    while ((syslog_buffer[syslog_buffer_dump] != 0) && (syslog_buffer_dump != syslog_buffer_head))
    {
      // Increment the dump tail within the bounds of the buffer.
      syslog_buffer_dump = (syslog_buffer_dump + 1) % SYSLOG_BUFFER_SIZE;
    }
  }
}

// Dump the buffered serial data to the telnet port.
static bool syslog_shell_dmesg(int argc, char **argv)
{
  // Lock the syslog mutex.
  if (syslog_mutex_id && (osMutexAcquire(syslog_mutex_id, osWaitForever) == osOK))
  {
    // Start at the dump tail.
    uint32_t index = syslog_buffer_dump;

    // Keep going until we reach the head.
    while (index != syslog_buffer_head)
    {
      // Get the next character to output.
      int ch = (int) syslog_buffer[index];

      // Output the character and substitue nulls with CR characters.
      shell_putc(ch == 0 ? '\n' : ch);

      // Increment the index within the bounds of the buffer.
      index = (index + 1) % SYSLOG_BUFFER_SIZE;
    }

    // Release the mutex.
    osMutexRelease(syslog_mutex_id);
  }

  return true;
}

// Send a message to syslog.
static bool syslog_shell_syslog(int argc, char **argv)
{
  bool help = false;

  // Parse the command.
  if (argc > 1)
  {
    if (!strcasecmp(argv[1], "clear"))
    {
      // Clear the syslog buffer.
      syslog_clear();
    }
    else if (!strcasecmp(argv[1], "echo"))
    {
      // Are we setting or just getting?
      if (argc > 2)
      {
        // Set the echo flag.
        syslog_set_echo(!strcasecmp(argv[2], "on"));
      }

      // Display the echo flag.
      shell_printf("echo: %s\n", syslog_get_echo() ? "on" : "off");
    }
    else if (!strcasecmp(argv[1], "level"))
    {
      // Are we setting or just getting?
      if (argc > 2)
      {
        int level = syslog_level;

        // Try to parse the level as a string, then as a number.
        if (!strcasecmp(argv[2], syslog_levels[0])) level = SYSLOG_EMERGENCY;
        else if (!strcasecmp(argv[2], syslog_levels[1])) level = SYSLOG_ALERT;
        else if (!strcasecmp(argv[2], syslog_levels[2])) level = SYSLOG_CRITICAL;
        else if (!strcasecmp(argv[2], syslog_levels[3])) level = SYSLOG_ERROR;
        else if (!strcasecmp(argv[2], syslog_levels[4])) level = SYSLOG_WARNING;
        else if (!strcasecmp(argv[2], syslog_levels[5])) level = SYSLOG_NOTICE;
        else if (!strcasecmp(argv[2], syslog_levels[6])) level = SYSLOG_INFO;
        else if (!strcasecmp(argv[2], syslog_levels[7])) level = SYSLOG_DEBUG;
        else level = atoi(argv[2]);

        // Set the logging level.
        syslog_set_level(level);
      }

      // Display the current level.
      shell_printf("level: %s (%d)\n", syslog_levels[syslog_get_level()], syslog_get_level());
    }
    else if (argc > 2)
    {
      int severity = atoi(argv[1]);

      // Handle various arguments.
      if (argc == 3)
        syslog_printf(severity, "%s", argv[2]);
      else if (argc == 4)
        syslog_printf(severity, "%s %s", argv[2], argv[3]);
      else if (argc == 5)
        syslog_printf(severity, "%s %s %s", argv[2], argv[3], argv[4]);
      else if (argc == 6)
        syslog_printf(severity, "%s %s %s %s", argv[2], argv[3], argv[4], argv[5]);
      else if (argc == 7)
        syslog_printf(severity, "%s %s %s %s %s", argv[2], argv[3], argv[4], argv[5], argv[6]);
      else if (argc == 8)
        syslog_printf(severity, "%s %s %s %s %s %s", argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
      else
        // We need help.
        help = true;
    }
    else
    {
      // We need help.
      help = true;
    }
  }
  else
  {
    // We need help.
    help = true;
  }

  // Do we need to display help?
  if (help)
  {
    shell_puts("  syslog 0-7 message\n");
    shell_puts("  syslog echo [on|off]\n");
    shell_puts("  syslog clear\n");
    shell_puts("  syslog level [emergency|alert|critical|error|warning|notice|info|debug|0-7]\n");
  }

  return true;
}

// Send syslog packets within the context of the tcpip thread.
static void syslog_send_callback(void *arg)
{
  // Get buffered messages until we get an empty buffer.
  while (!syslog_buffer_empty())
  {
    // Lock the syslog mutex.
    if (syslog_mutex_id && (osMutexAcquire(syslog_mutex_id, osWaitForever) == osOK))
    {
      // Get the next syslog message.
      syslog_next_message();

      // Only send a message if syslog is enabled and the buffer is a sane length.
      if (syslog_send_len > 8)
      {
#if USE_NETWORK_SYSLOG
        // Allocate the syslog packet buffer.
        struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, syslog_send_len, PBUF_REF);

        // Did the allocation succeed?
        if (p != NULL)
        {
          // Point to the payload to the syslog send buffer.
          p->payload = syslog_send_buffer;

          // Send the syslog message.
          udp_send(syslog_pcb, p);

          // Free the syslog packet buffer.
          pbuf_free(p);
        }
#endif
      }

      // Release the syslog mutex.
      osMutexRelease(syslog_mutex_id);
    }
  }
}

// Initialize within the context of the tcpip thread.
static void syslog_init_callback(void *arg)
{
  ip4_addr_t dest_ip_addr;

  // Get the address we are sending packets to.
  dest_ip_addr = syslog_config_address();

  // Initialize a new UDP control block.
  if ((syslog_pcb = udp_new()) != NULL)
  {
    // Bind the pcb to a local address.
    if (udp_bind(syslog_pcb, IP_ADDR_ANY, syslog_config_port()) == ERR_OK)
    {
      // Set up the connection.
      if (udp_connect(syslog_pcb, &dest_ip_addr, syslog_config_port()) == ERR_OK)
      {
        // Add the shell command.
        shell_add_command("dmesg", syslog_shell_dmesg);
        shell_add_command("syslog", syslog_shell_syslog);

        // We succeeded and syslog is now initialized.
        syslog_initialized = true;
      }
    }
  }
}

// Initialize log buffering.
void syslog_init(void)
{
  // Static mutex control block.
  static uint32_t syslog_mutex_cb[osRtxMutexCbSize/4U] __attribute__((section(".bss.os.mutex.cb")));

  // Initialize the mutex attributes. Note this is not a recursive mutex, but it does inherit priority.
  osMutexAttr_t syslog_mutex_attrs =
  {
    .name = "syslog",
    .attr_bits = osMutexPrioInherit,
    .cb_mem = syslog_mutex_cb,
    .cb_size = sizeof(syslog_mutex_cb)
  };

  // Create the syslog mutex.
  syslog_mutex_id = osMutexNew(&syslog_mutex_attrs);
  if (!syslog_mutex_id)
  {
    // Report error to console.
    outputf("SYSLOG: unable to create mutex\n");
  }

  // We need to initialize within the context of the tcpip thread.
  tcpip_callback(syslog_init_callback, NULL);
}

// Clear the syslog buffer.
void syslog_clear(void)
{
  // Sanity check we are initialized.
  if (!syslog_initialized) return;

  // Lock the syslog mutex.
  osMutexAcquire(syslog_mutex_id, osWaitForever);

  // Reset the buffer indices.
  syslog_buffer_head = 0;
  syslog_buffer_tail = 0;
  syslog_buffer_dump = 0;

  // Release the mutex.
  osMutexRelease(syslog_mutex_id);
}

// Set the syslog echo to console flag.
void syslog_set_echo(bool enable)
{
  // Set the syslog echo to console flag.
  syslog_console_echo = enable;
}

// Get the syslog echo to console flag.
bool syslog_get_echo(void)
{
  // Get the syslog echo to console flag.
  return syslog_console_echo;
}

// Set the syslog logging level.
void syslog_set_level(int severity)
{
  // Sanity check the severity.
  if (severity > SYSLOG_DEBUG) severity = SYSLOG_DEBUG;
  if (severity < SYSLOG_EMERGENCY) severity = SYSLOG_EMERGENCY;

  // Set the logging level.
  syslog_level = severity;
}

// Get the syslog logging level.
int syslog_get_level(void)
{
  return syslog_level;
}

// Output a formatted syslog message.
void syslog_printf(int severity, const char *format, ...)
{
  int i;
  int offset;
  int priority;
  va_list args;

  // Should we echo the syslog data to the console?
  if (syslog_console_echo && syslog_mutex_id && (osMutexAcquire(syslog_mutex_id, osWaitForever) == osOK))
  {
    // Format the data to the console.
    va_start(args, format);
    voutputf(format, args);
    va_end(args);
    outputf("\n");

    // Release the mutex.
    osMutexRelease(syslog_mutex_id);
  }

  // Sanity check we are initialized.
  if (!syslog_initialized) return;

  // Sanity check the severity.
  if (severity > SYSLOG_DEBUG) severity = SYSLOG_DEBUG;
  if (severity < SYSLOG_EMERGENCY) severity = SYSLOG_EMERGENCY;

  // Skip severities greater than our current logging level.
  if (severity > syslog_level) return;

  // Determine the priority.
  priority = (SYSLOG_LOCAL0 << 3) | severity;

  // Lock the syslog mutex.
  if (syslog_mutex_id && (osMutexAcquire(syslog_mutex_id, osWaitForever) == osOK))
  {
    // Place the syslog header into the send buffer.
    offset = snoutputf(syslog_send_buffer, sizeof(syslog_send_buffer), "<%d>", priority);
    offset += systime_log(syslog_send_buffer + offset, sizeof(syslog_send_buffer) - offset);

    // Format the data into the send buffer.
    va_start(args, format);
    offset += vsnoutputf(syslog_send_buffer + offset, sizeof(syslog_send_buffer) - offset, format, args);
    va_end(args);

    // Insert each character from the send buffer to the syslog buffer.
    for (i = 0; i < sizeof(syslog_send_buffer) && syslog_send_buffer[i]; ++i)
      syslog_putc(syslog_send_buffer[i]);

    // Insert the end-of-string character.
    syslog_putc(0);

    // Release the mutex.
    osMutexRelease(syslog_mutex_id);
  }

  // We need to send within the context of the tcpip thread.
  tcpip_callback(syslog_send_callback, NULL);
}

// System configurable syslog IP address.
__WEAK ip4_addr_t syslog_config_address(void)
{
  ip4_addr_t addr = { IPADDR_ANY };
  return addr;
}

// System configurable syslog port.
__WEAK uint16_t syslog_config_port(void)
{
  return 514;
}
