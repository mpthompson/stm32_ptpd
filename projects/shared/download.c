#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "rtx_os.h"
#include "shell.h"
#include "flash.h"
#include "tick.h"
#include "outputf.h"
#include "random.h"
#include "network.h"
#include "syslog.h"
#include "webclient.h"
#include "download.h"

#define DOWNLOAD_CRC_BUFLEN         64

// Download event values.
#define DOWNLOAD_EVENT_TIMER        0x0001u

// Download network up timeout in milliseconds.
#define DOWNLOAD_NETWORK_UP_TIMEOUT 5000

// Download states.
enum
{
  DOWNLOAD_STATE_WAIT = 0,
  DOWNLOAD_STATE_START_CRC,
  DOWNLOAD_STATE_WAIT_CRC,
  DOWNLOAD_STATE_PARSE_CRC,
  DOWNLOAD_STATE_ERASE_APP,
  DOWNLOAD_STATE_START_PROG,
  DOWNLOAD_STATE_START_DATA,
  DOWNLOAD_STATE_WAIT_DATA,
  DOWNLOAD_STATE_END_PROG,
  DOWNLOAD_STATE_RUN_APP,
  DOWNLOAD_STATE_ERROR,
  DOWNLOAD_STATE_COUNT
};

// Download mutex id.
static osMutexId_t download_mutex_id = NULL;

// Download thread id.
static osThreadId_t download_thread_id = NULL;

// Download event flags id.
static osEventFlagsId_t download_event_id = NULL;

// Download timer id.
static osTimerId_t download_timer_id;

// State of the firmware download state machine.
static uint32_t download_state = 0;
static uint32_t download_file_crc = 0;
static uint32_t download_file_len = 0;
static uint32_t download_crc_state = 0;
static uint32_t download_crc_buflen = 0;
static uint8_t download_crc_buffer[DOWNLOAD_CRC_BUFLEN];

// Start time in system ticks.
static uint32_t download_start_tick = 0;

// Callback used to download the firmware data.  This is called from the context
// of the TCP thread so be careful about interaction with the download thread.
static void download_data_callback(uint32_t status, const uint8_t *buffer, uint32_t buflen)
{
  // Lock the download mutex.
  osMutexAcquire(download_mutex_id, osWaitForever);

  // Did we receive an error?
  if (status == WEBCLIENT_STS_OK)
  {
    // Have we finished receiving data?
    if (!buflen)
    {
      // Yes. We are done so move to the next download state.
      download_state = DOWNLOAD_STATE_END_PROG;
    }
    else
    {
      // Program the buffer into application flash memory.
      flash_app_prog_write(buffer, buflen);
    }
  }
  else if (status == WEBCLIENT_STS_TIMEOUT)
  {
    // Log the timeout.
    syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: firmware file timeout");

    // Move to the error download state.
    download_state = DOWNLOAD_STATE_ERROR;
  }
  else if (status == WEBCLIENT_STS_NOTFOUND)
  {
    // Log the timeout.
    syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: firmware file not found");

    // Move to the error download state.
    download_state = DOWNLOAD_STATE_ERROR;
  }
  else
  {
    // Log the error.
    syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: firmware file download error %u", status);

    // Move to the error download state.
    download_state = DOWNLOAD_STATE_ERROR;
  }

  // Release the download mutex.
  osMutexRelease(download_mutex_id);
}

// Parses the firmware CRC and length value from the indicated buffer.
static void download_crc_parse(const uint8_t *buffer, uint32_t buflen)
{
  uint32_t crc = 0;
  uint32_t len = 0;
  const uint8_t *p = buffer;

  // Skip over leading space characters.
  while (*p && isspace(*p)) p++;

  // Determine the file CRC.
  while (*p && isdigit(*p)) crc = (crc * 10) + (*(p++) - '0');

  // Skip over separating space characters.
  while (*p && isspace(*p)) p++;

  // Determine the file length.
  while (*p && isdigit(*p)) len = (len * 10) + (*(p++) - '0');

  // Skip over terminating space characters.
  while (*p && isspace(*p)) p++;

  // We should be at the end of the buffer.
  if (*p == 0)
  {
    // Fill in the firmware download CRC and length.
    download_file_crc = crc;
    download_file_len = len;
  }
}

// Parse the firmware download CRC data buffer looking for known data.
// We expect only the first line of the CRC file to be filled in with
// the CRC value and the file length. For example:
//
// 3310644667 141164
//
static void download_crc_process(const uint8_t *buffer, uint32_t buflen)
{
  // Parse the buffer a single character at a time.
  while (buflen)
  {
    // Handle the parsing state.
    switch (download_crc_state)
    {
      case 0:
        // Only gather printable characters.
        if ((*buffer >= ' ') && (*buffer <= '~'))
        {
          // Be careful not to overflow the buffer.
          if (download_crc_buflen < (sizeof(download_crc_buffer) - 1))
          {
            // Insert the character into the buffer.
            download_crc_buffer[download_crc_buflen++] = *buffer;
          }
        }

        // Have we reached the end of the line?
        if (*buffer == '\n')
        {
          // Null terminate the buffer.
          download_crc_buffer[download_crc_buflen] = 0;

          // Move to the next state.
          download_crc_state = 1;
        }
        break;

      case 1:
        // Do nothing.  Only the first line is parsed.
        break;
    }

    // Decrement the length and increment the buffer.
    buflen -= 1;
    buffer += 1;
  }
}

// Callback used to parse download of firmware CRC file.  This is called from the
// context of the TCP thread so be careful about interaction with the download thread.
static void download_crc_callback(uint32_t status, const uint8_t *buffer, uint32_t buflen)
{
  // Lock the download mutex.
  osMutexAcquire(download_mutex_id, osWaitForever);

  // Did we receive an error?
  if (status == WEBCLIENT_STS_OK)
  {
    // Have we finished receiving data?
    if (!buflen)
    {
      // Yes. We are done so move to the next download state.
      download_state = DOWNLOAD_STATE_PARSE_CRC;
    }
    else
    {
      // Process the downloaded CRC data buffer.
      download_crc_process(buffer, buflen);
    }
  }
  else if (status == WEBCLIENT_STS_TIMEOUT)
  {
    // Log the timeout.
    syslog_printf(SYSLOG_NOTICE, "DOWNLOAD: CRC file timeout");

    // Assume we should just run existing application firmware.
    download_state = DOWNLOAD_STATE_RUN_APP;
  }
  else if (status == WEBCLIENT_STS_NOTFOUND)
  {
    // Log the timeout.
    syslog_printf(SYSLOG_NOTICE, "DOWNLOAD: CRC file not found");

    // Assume we should just run existing application firmware.
    download_state = DOWNLOAD_STATE_RUN_APP;
  }
  else
  {
    // Log the error.
    syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: CRC file download error %u", status);

    // Assume we should just run existing application firmware.
    download_state = DOWNLOAD_STATE_RUN_APP;
  }

  // Release the download mutex.
  osMutexRelease(download_mutex_id);
}

// Process the firmware download timer.
static void download_process_timer(void)
{
  uint32_t app_crc = 0;
  uint16_t port = 0;
  ip4_addr_t addr = { 0 };
  static char get_path[64];

  // Handle the firmware download state.
  switch (download_state)
  {
    case DOWNLOAD_STATE_WAIT:

      // Is the network up?
      if (network_is_up())
      {
        // Yes. Move to the next state to initiate download.
        download_state = DOWNLOAD_STATE_START_CRC;
      }

      // Have we waited long enough for the network to come up?
      if (tick_to_milliseconds(osKernelGetTickCount() - download_start_tick) > DOWNLOAD_NETWORK_UP_TIMEOUT)
      {
        // Log the timeout of network up.
        syslog_printf(SYSLOG_NOTICE, "DOWNLOAD: timed out waiting for network up");

        // Assume we should just run existing application firmware.
        download_state = DOWNLOAD_STATE_RUN_APP;
      }
      break;

    case DOWNLOAD_STATE_START_CRC:
      // Get the address and port to download firmware from.
      addr = download_config_address();
      port = download_config_port();

      // Reset the download CRC state.
      download_crc_state = 0;
      download_crc_buflen = 0;
      download_file_crc = 0;
      download_file_len = 0;

      // Create the path to the firmware CRC file.
      snoutputf(get_path, sizeof(get_path), "%s.crc", download_config_firmware_name());

      // Initiate the download of the firmware CRC file.
      if (webclient_get(addr, port, get_path, download_crc_callback) != 0)
      {
        // Log the critical error.
        syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: error getting file '%s'", get_path);

        // Move to the error state.
        download_state = DOWNLOAD_STATE_ERROR;
      }
      else
      {
        // Log the start of getting the CRC file.
        syslog_printf(SYSLOG_NOTICE, "DOWNLOAD: getting '%s'", get_path);

        // Move to the next state to wait for download to complete.
        download_state = DOWNLOAD_STATE_WAIT_CRC;
      }
      break;

    case DOWNLOAD_STATE_WAIT_CRC:
      // Stay in this state while the firmware CRC file downloads.
      break;

    case DOWNLOAD_STATE_PARSE_CRC:
      // Make sure the CRC buffer is null terminated.
      download_crc_buffer[download_crc_buflen] = 0;

      // Parse the application firmware CRC and file length.
      download_crc_parse(download_crc_buffer, download_crc_buflen);

      // We should have a non-zero filelength.
      if (download_file_len != 0)
      {
        // Log the firmware CRC and length.
        syslog_printf(SYSLOG_NOTICE, "DOWNLOAD: host firmware crc %u with size %u", download_file_crc, download_file_len);

        // Determine the existing application firmware CRC.
        app_crc = flash_app_crc(download_file_len);

        // Does the firmware CRC and application CRC match?
        if (download_file_crc == app_crc)
        {
          // Log the firmware matches.
          syslog_printf(SYSLOG_NOTICE, "DOWNLOAD: firmware crc %u match, bypassing download", app_crc);

          // We should run the application firmware.
          download_state = DOWNLOAD_STATE_RUN_APP;
        }
        else
        {
          // Log the firmware matches.
          syslog_printf(SYSLOG_NOTICE, "DOWNLOAD: firmware crc %u mismatch, attempting download", app_crc);

          // Log the erasing of firmware.
          syslog_printf(SYSLOG_NOTICE, "DOWNLOAD: erasing application flash memory");

          // Proceed to the next state.
          download_state = DOWNLOAD_STATE_ERASE_APP;
        }
      }
      else
      {
        // Log the critical error.
        syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: error parsing '%s.crc'", download_config_firmware_name());

        // Move to the error state.
        download_state = DOWNLOAD_STATE_ERROR;
      }
      break;

    case DOWNLOAD_STATE_ERASE_APP:

      // Erase the application flash memory.
      if (flash_app_erase() != 0)
      {
        // Log the critical error.
        syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: error erasing application flash memory");

        // Move to the error state.
        download_state = DOWNLOAD_STATE_ERROR;
      }
      else
      {
        // Proceed to the next state.
        download_state = DOWNLOAD_STATE_START_PROG;
      }
      break;

    case DOWNLOAD_STATE_START_PROG:

      // Start the programming application flash memory.
      if (flash_app_prog_start(download_file_len) != 0)
      {
        // Log the error.
        syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: error programming application flash memory");

        // Move to the critical error state.
        download_state = DOWNLOAD_STATE_ERROR;
      }
      else
      {
        // Proceed to the next state.
        download_state = DOWNLOAD_STATE_START_DATA;
      }
      break;

    case DOWNLOAD_STATE_START_DATA:

      // Get the address and port to download firmware from.
      addr = download_config_address();
      port = download_config_port();

      // Create the path to the firmware CRC file.
      snoutputf(get_path, sizeof(get_path), "%s.bin", download_config_firmware_name());

      // Initiate the download of the firmware data file.
      if (webclient_get(addr, port, get_path, download_data_callback) != 0)
      {
        // End the programming of application flash memory
        flash_app_prog_end();

        // Log the error.
        syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: error getting '%s'", get_path);

        // Move to the error state.
        download_state = DOWNLOAD_STATE_ERROR;
      }
      else
      {
        // Log the start of getting the data file.
        syslog_printf(SYSLOG_NOTICE, "DOWNLOAD: getting '%s'", get_path);

        // Move to the next state to wait for download to complete.
        download_state = DOWNLOAD_STATE_WAIT_DATA;
      }
      break;

    case DOWNLOAD_STATE_WAIT_DATA:

      // Stay in this state while the firmware data file downloads.
      break;

    case DOWNLOAD_STATE_END_PROG:

      // End the programming of application flash memory
      flash_app_prog_end();

      // Determine the existing application firmware CRC.
      app_crc = flash_app_crc(download_file_len);

      // Does the firmware CRC and application CRC match?
      if (download_file_crc == app_crc)
      {
        // Yes. We can start application firmware.
        download_state = DOWNLOAD_STATE_RUN_APP;
      }
      else
      {
        // No. Log the error.
        syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: error flashing '%s.bin'", download_config_firmware_name());

        // Move to the error state.
        download_state = DOWNLOAD_STATE_ERROR;
      }
      break;

    case DOWNLOAD_STATE_RUN_APP:

      // Log the start of application firmware.
      syslog_printf(SYSLOG_NOTICE, "DOWNLOAD: running application firmware");

      // Run the bootloader.
      if (flash_app_run() != 0)
      {
        syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: failed to run application firmware");
      }

      // Move to the error state.
      download_state = DOWNLOAD_STATE_ERROR;
      break;

    case DOWNLOAD_STATE_ERROR:
      // Just stay in this state.
      break;
  }
}

// Firmare download periodic timer handler. This is processed on the timer thread.
static void download_timer_handler(void *p_context)
{
  // Signal the periodic timer elapsed.
  osEventFlagsSet(download_event_id, DOWNLOAD_EVENT_TIMER);
}

// Firmware download processing thread.
static void download_thread(void *arg)
{
  ip4_addr_t address;

  // Get our current IP address.
  address = network_get_address();

  // Wait until we have been assigned an IP address.
  while (address.addr == 0)
  {
    // Delay a random amount.
    random_delay(1000, 2000);

    // Get our current IP address.
    address = network_get_address();
  }

  // Wait a random time before starting download.
  random_delay(1000, 2000);

  // Save the start time in ticks.
  download_start_tick = osKernelGetTickCount();

  // Start the periodic timer callback every 100ms.
  osTimerStart(download_timer_id, 100);

  // Loop forever.
  for (;;)
  {
    // Wait for a timer event.
    osEventFlagsWait(download_event_id, DOWNLOAD_EVENT_TIMER, osFlagsWaitAny, osWaitForever);

    // Lock the download mutex.
    osMutexAcquire(download_mutex_id, osWaitForever);

    // Process the timer event.
    download_process_timer();

    // Release the download mutex.
    osMutexRelease(download_mutex_id);
  }
}

// Firmware download shell utility.
static bool download_shell_download(int argc, char **argv)
{
  // Report on the download state.
  switch (download_state)
  {
    case DOWNLOAD_STATE_START_CRC:
      shell_printf("Download State: START_CRC (%u)\n", download_state);
      break;
    case DOWNLOAD_STATE_WAIT_CRC:
      shell_printf("Download State: WAIT_CRC (%u)\n", download_state);
      break;
    case DOWNLOAD_STATE_PARSE_CRC:
      shell_printf("Download State: PARSE_CRC (%u)\n", download_state);
      break;
    case DOWNLOAD_STATE_ERASE_APP:
      shell_printf("Download State: ERASE_APP (%u)\n", download_state);
      break;
    case DOWNLOAD_STATE_START_PROG:
      shell_printf("Download State: START_PROG (%u)\n", download_state);
      break;
    case DOWNLOAD_STATE_START_DATA:
      shell_printf("Download State: START_DATA (%u)\n", download_state);
      break;
    case DOWNLOAD_STATE_WAIT_DATA:
      shell_printf("Download State: WAIT_DATA (%u)\n", download_state);
      break;
    case DOWNLOAD_STATE_END_PROG:
      shell_printf("Download State: END_PROG (%u)\n", download_state);
      break;
    case DOWNLOAD_STATE_RUN_APP:
      shell_printf("Download State: RUN_APP (%u)\n", download_state);
      break;
    case DOWNLOAD_STATE_ERROR:
      shell_printf("Download State: START_ERROR (%u)\n", download_state);
      break;
    default:
      shell_printf("Download State: UNKNOWN (%u)\n", download_state);
      break;
  }

  return true;
}

// Initiate firmware download processes.
void download_init(void)
{
  // Static mutex, timer, event and thread control blocks.
  static uint32_t download_mutex_cb[osRtxMutexCbSize/4U] __attribute__((section(".bss.os.mutex.cb")));
  static uint32_t download_timer_cb[osRtxTimerCbSize/4U] __attribute__((section(".bss.os.timer.cb")));
  static uint32_t download_event_cb[osRtxEventFlagsCbSize/4U] __attribute__((section(".bss.os.evflags.cb")));
  static uint32_t download_thread_cb[osRtxThreadCbSize/4U] __attribute__((section(".bss.os.thread.cb")));

  // Download mutex attributes. Note the mutex is not recursive.
  osMutexAttr_t console_mutex_attrs = { .name = "download", .attr_bits = 0U,
                                        .cb_mem = download_mutex_cb, .cb_size = sizeof(download_mutex_cb) };

  // Create the download mutex.
  download_mutex_id = osMutexNew(&console_mutex_attrs);
  if (download_mutex_id == NULL)
  {
    syslog_printf(SYSLOG_ERROR, "DOWNLOAD: error creating mutex");
    return;
  }

  // Download control event attributes.
  osEventFlagsAttr_t download_event_attrs = { .name = "download", .attr_bits = 0U,
                                              .cb_mem = download_event_cb, .cb_size = sizeof(download_event_cb) };

  // Create the console event flags.
  download_event_id = osEventFlagsNew(&download_event_attrs);
  if (download_event_id == NULL)
  {
    // Log the critical error.
    syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: error creating event flags");
    return;
  }

  // Download timer attributes.
  osTimerAttr_t download_timer_attrs = { .name = "download", .attr_bits = 0U,
                                         .cb_mem = download_timer_cb, .cb_size = sizeof(download_timer_cb) };

  // Create the download timer.
  download_timer_id = osTimerNew(download_timer_handler, osTimerPeriodic, NULL, &download_timer_attrs);
  if (download_timer_id == NULL)
  {
    // Log the critical error.
    syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: error creating periodic timer");
    return;
  }

  // Download thread attributes.
  osThreadAttr_t control_thread_attrs = { .name = "download", .attr_bits = 0U,
                                          .cb_mem = download_thread_cb, .cb_size = sizeof(download_thread_cb),
                                          .priority = osPriorityNormal };

  // Initialize the download thread.
  download_thread_id = osThreadNew(download_thread, NULL, &control_thread_attrs);
  if (download_thread_id == NULL)
  {
    // Log the critical error.
    syslog_printf(SYSLOG_CRITICAL, "DOWNLOAD: error creating processing thread");
    return;
  }

  // Add the download shell command.
  shell_add_command("download", download_shell_download);
}

// System configurable download IP address.
__WEAK ip4_addr_t download_config_address(void)
{
  ip4_addr_t addr = { IPADDR_ANY };
  return addr;
}

// System configurable download port.
__WEAK uint16_t download_config_port(void)
{
  return 80;
}

// System configurable download firmware name.
__WEAK const char *download_config_firmware_name(void)
{
  return "firmware";
}
