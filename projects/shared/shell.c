#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "cmsis_os2.h"
#include "rtx_os.h"
#include "outputf.h"
#include "delay.h"
#include "syslog.h"
#include "uptime.h"
#include "shell.h"

#define SHELL_MAX_CLIENT_COUNT              4
#define SHELL_MAX_COMMAND_COUNT             64
#define SHELL_MAX_COMMAND_ARGS              16

// Structure used to store pointers to command names and functions.
typedef struct shell_command_s
{
  const char *name;
  shell_func_t func;
} shell_command_t;

// Client count and list of commands.
static uint32_t shell_client_count = 0;
static shell_client_t shell_clients[SHELL_MAX_CLIENT_COUNT];

// Command count and list of commands.
static uint32_t shell_command_count = 0;
static shell_command_t shell_commands[SHELL_MAX_COMMAND_COUNT];

// Shell mutex protection.
osMutexId_t shell_mutex_id = NULL;

// Find the shell client matching the thread id.
static shell_client_t *shell_get_client(void *thread_id)
{
  shell_client_t *client = NULL;

  // Acquire the mutex.
  if (shell_mutex_id && (osMutexAcquire(shell_mutex_id, osWaitForever) == osOK))
  {
    // Loop over each client.
    for (uint32_t i = 0; (client == NULL) && (i < shell_client_count); ++i)
    {
      // Return the client if the thread ids match.
      if (thread_id == shell_clients[i].thread_id)
      {
        // Set the client to be returned.
        client = &shell_clients[i];
      }
    }

    // Release the mutex.
    osMutexRelease(shell_mutex_id);
  }

  return client;
}

// Exit the shell interpreter.
static bool shell_exit(int argc, char **argv)
{
  // Returning false indicates shell processing should be terminated.
  return false;
}

// Print help.
static bool shell_help(int argc, char **argv)
{
  uint32_t i;

  // Loop over each shell command.
  for (i = 0; i < shell_command_count; ++i)
  {
    shell_printf("%s\n", shell_commands[i].name);
  }

  return true;
}

// Shell clear.
static bool shell_clear(int argc, char **argv)
{
  // Create the clear terminal, home cursor character sequence.
  const char buf[9] = { 0x1B, '[', '2', 'J', 0x1b, '[', ';', 'H', 0x00 };

  // Output to the terminal.
  shell_puts(buf);

  return true;
}

// Shell delay.
static bool shell_delay(int argc, char **argv)
{
  // Default is delay one second.
  uint32_t milliseconds = 1000U;

  // Set the millisecond delay.
  if (argc > 1)
  {
    // Parse the milliseconds and prevent too long of a delay.
    milliseconds = (uint32_t) strtol(argv[1], NULL, 0);
    if (milliseconds > 60000U) milliseconds = 60000U;
  }

  // Delay for the milliseconds.
  delay_ms(milliseconds);

  return true;
}

// Reboot the system.
static bool shell_reboot(int argc, char **argv)
{
  // Get the shell client thread matching the current thread.
  shell_client_t *client = shell_get_client(osThreadGetId());

  // If we found the client call the client reboot function.
  if (client && client->reboot) client->reboot();

  // Exit the shell interpreter.
  return false;
}

// Shutdown the system.
static bool shell_shutdown(int argc, char **argv)
{
  // Get the shell client thread matching the current thread.
  shell_client_t *client = shell_get_client(osThreadGetId());

  // If we found the client call the client shutdown function.
  if (client && client->shutdown) client->shutdown();

  // Exit the shell interpreter.
  return false;
}

// Report the current uptime.
static bool shell_uptime(int argc, char **argv)
{
  uint32_t uptime;
  uint32_t seconds;
  uint32_t minutes;
  uint32_t hours;

  // Get the uptime in seconds.
  uptime = uptime_get() / 1000;

  // Get the hours, minutes and seconds.
  seconds = uptime % 60;
  minutes = (uptime / 60) % 60;
  hours = (uptime / 3600);

  // Report the uptime.
  shell_printf("Uptime: %u:%02u:%02u\n", hours, minutes, seconds);

  return true;
}

// Report the firmware version.
static bool shell_version(int argc, char **argv)
{
  // Report the firmware version information string.
  shell_printf("%s\n", shell_config_version());

  return true;
}

// Parse out the next non-space word from a string.
// str    Pointer to pointer to the string
// word   Pointer to pointer of next word.
// Returns 0:Failed, 1:Successful
static int shell_parse(char **str, char **word)
{
  // Skip leading spaces.
  while (**str && isspace((unsigned char) **str)) (*str)++;

  // Set the word.
  *word = *str;

  // Skip non-space characters.
  while (**str && !isspace((unsigned char) **str)) (*str)++;

  // Null terminate the word.
  if (**str) *(*str)++ = 0;

  return (*str != *word) ? 1 : 0;
}

// For sorting commands.
static int shell_cmp(const void *elem1, const void *elem2)
{
  shell_command_t *f = (shell_command_t *) elem1;
  shell_command_t *s = (shell_command_t *) elem2;
  return strcasecmp(f->name, s->name);
}

// Initialize shell command processing.
void shell_init(void)
{
  // Static control block.
  static uint32_t shell_mutex_cb[osRtxMutexCbSize/4U] __attribute__((section(".bss.os.mutex.cb")));

  // Initialize the mutex attributes. Note we enable the recursive mutex attribute.
  osMutexAttr_t shell_mutex_attrs =
  {
    .name = "shell",
    .attr_bits = osMutexRecursive,
    .cb_mem = shell_mutex_cb,
    .cb_size = sizeof(shell_mutex_cb)
  };

  // Create a mutex to be locked when accessing the shell structures.
  shell_mutex_id = osMutexNew(&shell_mutex_attrs);
  if (shell_mutex_id == NULL)
  {
    syslog_printf(SYSLOG_ERROR, "SHELL: cannot create mutex");
  }

  // Insert some of the default commands.
  shell_add_command("exit", shell_exit);
  shell_add_command("help", shell_help);
  shell_add_command("clear", shell_clear);
  shell_add_command("delay", shell_delay);
  shell_add_command("reboot", shell_reboot);
  shell_add_command("shutdown", shell_shutdown);
  shell_add_command("uptime", shell_uptime);
  shell_add_command("version", shell_version);
}

// Add a shell client.
int shell_add_client(shell_client_t *client)
{
  // Assume we fail.
  int success = -1;

  // Acquire the mutex.
  if (shell_mutex_id && (osMutexAcquire(shell_mutex_id, osWaitForever) == osOK))
  {
    // Sanity check the shell client count.
    if (shell_client_count < SHELL_MAX_CLIENT_COUNT)
    {
      // Insert the shell client into the list of clients.
      shell_clients[shell_client_count].thread_id = client->thread_id;
      shell_clients[shell_client_count].hasc = client->hasc;
      shell_clients[shell_client_count].getc = client->getc;
      shell_clients[shell_client_count].putc = client->putc;
      shell_clients[shell_client_count].flush = client->flush;
      shell_clients[shell_client_count].reboot = client->reboot;
      shell_clients[shell_client_count].shutdown = client->shutdown;

      // Increment the client count.
      shell_client_count += 1;
    }

    // Release the mutex.
    osMutexRelease(shell_mutex_id);
  }

  return success;
}

// Add a shell command. Should only be called during system initialization.
int shell_add_command(const char *name, shell_func_t func)
{
  // Assume we fail.
  int success = -1;

  // Acquire the mutex.
  if (shell_mutex_id && (osMutexAcquire(shell_mutex_id, osWaitForever) == osOK))
  {
    // Sanity check the shell command count.
    if (shell_command_count < SHELL_MAX_COMMAND_COUNT)
    {
      // Insert the shell command into the list of commands.
      shell_commands[shell_command_count].name = name;
      shell_commands[shell_command_count].func = func;

      // Increment the command count.
      shell_command_count += 1;

      // Sort the shell commands by name.
      qsort(shell_commands, shell_command_count, sizeof(shell_command_t), shell_cmp);

      // We succeeded.
      success = 0;
    }
    else
    {
      // Report error adding shell command.
      syslog_printf(SYSLOG_ERROR, "SHELL: cannot add command '%s'", name);
    }

    // Release the mutex.
    osMutexRelease(shell_mutex_id);
  }

  return success;
}

// Parse the command line into argc and argv values.  This function is destructive
// to the command line so it must not be a constant string.
void shell_parse_arguments(char *cmdline, int *argc, char *argv[], int argv_count)
{
  int i;

  // Start with zero arguments.
  *argc = 0;

  // Parse the command and any arguments into an array.
  for (i = 0; i < argv_count; ++i)
  {
    // Parse the next space-separated word.  This inserts null
    // characters into the command line as they are parsed.
    shell_parse(&cmdline, &argv[i]);

    // Increment the argument count if a word was parsed.
    if (*argv[i] != 0) *argc = *argc + 1;
  }
}

// Process the the shell command. This function must remain re-entrant.
// If this returns true if the shell should be exited.
bool shell_command(char *cmdline)
{
  int argc = 0;
  char *argv[SHELL_MAX_COMMAND_ARGS];
  bool exit_shell = false;
  shell_command_t search;
  shell_command_t *command;
  shell_func_t func = NULL;

  // Parse the into a sequence of space separated arguments.
  shell_parse_arguments(cmdline, &argc, argv, SHELL_MAX_COMMAND_ARGS);

  // Acquire the mutex during the command search.
  if (shell_mutex_id && (osMutexAcquire(shell_mutex_id, osWaitForever) == osOK))
  {

    // Search for a matching shell command.
    search.name = argv[0];
    search.func = NULL;
    command = (shell_command_t *) bsearch(&search, shell_commands,
                                          shell_command_count,
                                          sizeof(shell_command_t), shell_cmp);

    // Did we find a shell command?
    if (command)
    {
      // Yes. Get the shell command function to be called.
      func = command->func;
    }

    // Release the mutex after the command search.
    osMutexRelease(shell_mutex_id);
  }

  // Do we have a shell command function to be called?
  if (func != NULL)
  {
    // Yes. Call the shell command function setting the flag whether to exit the shell.
    exit_shell = (command->func)(argc, argv) ? false : true;
  }
  else
  {
    // Report the shell command function not found.
    if (strlen(argv[0]) > 0) shell_printf("Unknown command: %s\n", argv[0]);
  }

  // Return true if we should exit the shell.
  return exit_shell;
}

// Returns 1 if there is a next character to be read from the shell, 0 if
// not or -1 on end of stream or error.
int shell_hasc(void)
{
  int ch = -1;

  // Get the shell client thread matching the current thread.
  shell_client_t *client = shell_get_client(osThreadGetId());

  // If we found the client call the client hasc function.
  if (client && client->hasc) ch = client->hasc();

  return ch;
}

// Returns the next character read from the shell as an unsigned char
// cast to an int or -1 on end of stream or error.
int shell_getc(void)
{
  int ch = -1;

  // Get the shell client thread matching the current thread.
  shell_client_t *client = shell_get_client(osThreadGetId());

  // If we found the client call the client getc function.
  if (client && client->getc) ch = client->getc();

  return ch;
}

// Shell raw character output.
void shell_putc_raw(int c)
{
  // Get the shell client thread matching the current thread.
  shell_client_t *client = shell_get_client(osThreadGetId());

  // If we found the client call the client putc function.
  if (client && client->putc) client->putc(c);
}

// Shell character output.
void shell_putc(int c)
{
  // If needed, convert LF to CRLF 
  if (c == '\n') shell_putc_raw('\r');

  // Send the character.
  shell_putc_raw(c);
}

// Shell string output.
void shell_puts(const char *str)
{
  // Get the shell client thread matching the current thread.
  shell_client_t *client = shell_get_client(osThreadGetId());

  // If we found the client call the client putc function.
  if (client && client->putc)
  {
    // Send each character in the string buffer with newline conversion.
    while (*str) { if (*str == '\n') client->putc('\r'); client->putc(*(str++)); }
  }
}

// Shell formatted data output.
void shell_printf(const char *fmt, ...)
{
  va_list arp;

  // Initialize the variable argument list.
  va_start(arp, fmt);

  // Get the shell client thread matching the current thread.
  shell_client_t *client = shell_get_client(osThreadGetId());

  // Output formatted data to the client putc function.
  if (client && client->putc) vfoutputf(client->putc, fmt, arp);

  // We are done with the variable argument list.
  va_end(arp);
}

// Shell data dump output.
void shell_pdump(const void *buffer, size_t buflen, uint32_t address, uint32_t width)
{
  // Print the address.
  shell_printf("%08X:", address);

  // Handle the different widths.
  if (width == sizeof(uint8_t))
  {
    const uint8_t *bp = (const uint8_t *) buffer;
    // Hex dump.
    for (uint32_t i = 0; i < buflen; ++i)
      shell_printf(" %02X", bp[i]);
    shell_putc(' ');
    // ASCII dump.
    for (uint32_t i = 0; i < buflen; ++i)
      shell_putc((bp[i] >= ' ' && bp[i] <= '~') ? bp[i] : '.');
  }
  else if (width == sizeof(uint16_t))
  {
    const uint16_t *sp = (const uint16_t *) buffer;
    // Hex dump.
    for (uint32_t i = 0; i < buflen; ++i)
      shell_printf(" %04X", sp[i]);
  }
  else if (width == sizeof(uint32_t))
  {
    const uint32_t *lp = (const uint32_t *) buffer;
    // Hex dump.
    for (uint32_t i = 0; i < buflen; ++i)
      shell_printf(" %08X", lp[i]);
  }
  shell_putc('\n');
}

// Shell flush output.
void shell_flush(void)
{
  // Get the shell client thread matching the current thread.
  shell_client_t *client = shell_get_client(osThreadGetId());

  // If we found the client call the client flush function.
  if (client && client->flush) client->flush();
}

// System configurable shell version string.
__WEAK const char *shell_config_version(void)
{
  return "Version " __DATE__ " " __TIME__;
}

