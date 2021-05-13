#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "stm32f4xx.h"
#include "lwip/sys.h"
#include "log.h"
#include "shell.h"

// Size of the serial buffer.
#define LOG_BUFFER_SIZE                 8192

static sys_mutex_t log_mutex;

// Buffered log structures.
static uint32_t log_buffer_head = 0;
static uint32_t log_buffer_tail = 0;
static char log_buffer[LOG_BUFFER_SIZE];

// Initialize log buffering.
void log_init(void)
{
  // Create the log mutex.
  sys_mutex_new(&log_mutex);

  // Initialize the buffer.
  log_buffer_tail = 0;
  log_buffer_head = 0;
}

// Put a character into the log.
void log_putc(int ch)
{
  // Grab the mutex.
  sys_mutex_lock(&log_mutex);

  // Place the character at the head of the buffer.
  log_buffer[log_buffer_head] = (char) ch;

  // Increment the message head.
  log_buffer_head += 1;

  // Keep within the bounds of the buffer.
  if (log_buffer_head >= LOG_BUFFER_SIZE) log_buffer_head = 0;

  // Have we collided with the tail indicating the buffer is full.
  if (log_buffer_head == log_buffer_tail)
  {
    char ch;

    // Get the current tail character.
    ch = log_buffer[log_buffer_tail];

    // Increment the tail.
    log_buffer_tail += 1;

    // Keep within the bounds of the buffer.
    if (log_buffer_tail >= LOG_BUFFER_SIZE) log_buffer_tail = 0;

    // Move the tail forward past the next newline character.
    while ((ch != '\n') && (log_buffer_tail != log_buffer_head))
    {
      // Get the current tail character.
      ch = log_buffer[log_buffer_tail];

      // Increment the tail.
      log_buffer_tail += 1;

      // Keep within the bounds of the buffer.
      if (log_buffer_tail >= LOG_BUFFER_SIZE) log_buffer_tail = 0;
    }
  }

  // Release the mutex.
  sys_mutex_unlock(&log_mutex);
}

void log_puts(const char *str)
{
  // Log each character in the string.
  while (*str) log_putc(*(str++));
}

void log_printf(const char *fmt, ...)
{
  char c;
  char d;
  char *p;
  char s[16];
  unsigned int f;
  unsigned int r;
  unsigned int i;
  unsigned int j;
  unsigned int w;
  unsigned long v;
  va_list arp;

  va_start (arp, fmt);
  for (;;)
  {
    // Get a character.
    c = *(fmt++);
    // End of format?
    if (!c) break;
    // Pass through if it not a % sequence.
    if (c != '%')
    {
      log_putc(c);
      continue;
    }
    f = 0;
    // Get first character of % sequence.
    c = *(fmt++);
    // Flag: '0' padded.
    if (c == '0')
    {
      f = 1;
      c = *(fmt++);
    }
    else
    {
      // Flag: left justified.
      if (c == '-')
      {
        f = 2;
        c = *(fmt++);
      }
    }
    for (w = 0; c >= '0' && c <= '9'; c = *(fmt++))  /* Minimum width */
      w = w * 10 + c - '0';
    // Prefix: Size is long int.
    if (c == 'l' || c == 'L')
    {
      f |= 4;
      c = *(fmt++);
    }
    // End of format.
    if (!c) break;
    d = c;
    if (d >= 'a') d -= 0x20;
    // Type is...
    switch (d) {
    // String.
    case 'S' :
      p = va_arg(arp, char*);
      for (j = 0; p[j]; j++);
      while (!(f & 2) && j++ < w) log_putc(' ');
      while (*p) log_putc(*(p++));
      while (j++ < w) log_putc(' ');
      continue;
    // Character.
    case 'C' :
      log_putc((char) va_arg(arp, int));
      continue;
    // Binary.
    case 'B' :
      r = 2;
      break;
    // Octal.
    case 'O' :
      r = 8;
      break;
    // Signed decimal.
    case 'D' :
    // Unsigned decimal.
    case 'U' :
      r = 10;
      break;
    // Hexadecimal.
    case 'X' :
      r = 16;
      break;
    // Unknown type (passthrough).
    default:
      log_putc(c);
      continue;
    }
    // Get an argument and put it in a numeral.
    v = (f & 4) ? va_arg(arp, long) : ((d == 'D') ? (long) va_arg(arp, int) : (long) va_arg(arp, unsigned int));
    if (d == 'D' && (v & 0x80000000)) {
      v = 0 - v;
      f |= 8;
    }
    i = 0;
    do {
      d = (char)(v % r); v /= r;
      if (d > 9) d += (c == 'x') ? 0x27 : 0x07;
      s[i++] = d + '0';
    } while (v && i < sizeof(s));
    if (f & 8) s[i++] = '-';
    j = i; d = (f & 1) ? '0' : ' ';
    while (!(f & 2) && j++ < w) log_putc(d);
    do log_putc(s[--i]); while(i);
    while (j++ < w) log_putc(' ');
  }
  va_end(arp);
}

// Dump the buffered serial data to the shell output.
void log_dump_to_shell(void)
{
  uint32_t index;

  // Grab the mutex.
  sys_mutex_lock(&log_mutex);

  // Start at the tail.
  index = log_buffer_tail;

  // Keep going until we reach the head.
  while (index != log_buffer_head)
  {
    // Output the character to the telnet session.
    shell_putc(log_buffer[index]);

    // Increment the index.
    index += 1;

    // Keep within the bounds of the buffer.
    if (index >= LOG_BUFFER_SIZE) index = 0;
  }

  // Release the mutex.
  sys_mutex_unlock(&log_mutex);
}

