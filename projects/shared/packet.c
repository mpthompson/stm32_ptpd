#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "packet.h"

// Parse the packet into delimiter separated arguments. This function is destructive
// to the packet so it must not be a constant string. Returns the number of arguments
// parsed in the packet.
int packet_parse_args(char *packet, char *argv[], int max_argc, char delimiter)
{
  int argc = 0;

  // Parse the delimited arguments from the packet.
  for (int i = 0; i < max_argc; i++)
  {
    // Set the current argument.
    argv[i] = packet;

    // Increment the argument count if not at the end of the packet.
    if (*packet) argc++;

    // Skip non-delimiter characters.
    while (*packet && (*packet != delimiter)) packet++;

    // Null terminate the argument if not at the end of the packet.
    if (*packet) *(packet++) = 0;
  }

  return argc;
}

// Parse the argument with the format seconds.nanoseconds into a timestamp.
int64_t packet_parse_timestamp(char *arg)
{
  char *argv[2];

  // Set the default value of zero.
  int64_t timestamp = 0L;

  // Parse into two parts delimited by a period.
  if (packet_parse_args(arg, argv, 2, '.') == 2)
  {
    int32_t seconds = strtol(argv[0], NULL, 10);
    int32_t nanoseconds = strtol(argv[1], NULL, 10);

    // Create the 64-bit timestamp from seconds and nanoseconds.
    timestamp = (((int64_t) seconds) * 1000000000) + nanoseconds;
  }

  return timestamp;
}

// Write the character into the packet buffer.
char *packet_write_char(char *buffer, char *bufend, char ch, char separator)
{
  // Sanity check the buffer.
  if (buffer < bufend)
  {
    // Write as much of the string as possible into the buffer.
    if (buffer < bufend)
      *(buffer++) = ch;

    // Append the field separator.
    if (buffer < bufend)
      *(buffer++) = separator;

    // Null terminate the buffer.
    *(buffer) = 0;
  }

  return buffer;
}

// Write the string into the packet buffer.
char *packet_write_string(char *buffer, char *bufend, char *str, char separator)
{
  // Sanity check the buffer.
  if (buffer < bufend)
  {
    // Write as much of the string as possible into the buffer.
    while (*str && (buffer < bufend))
    {
      *(buffer++) = *(str++);
    }

    // Append the field separator.
    if (buffer < bufend)
      *(buffer++) = separator;

    // Null terminate the buffer.
    *(buffer) = 0;
  }

  return buffer;
}

// Write the float value into the packet buffer.
char *packet_write_float(char *buffer, char *bufend, float value, char separator)
{
  // Sanity check the buffer.
  if (buffer < bufend)
  {
    // Handle NaN.
    if (isnan(value))
    {
      return packet_write_string(buffer, bufend, "nan", separator);
    }

    // Handle the negative sign.
    if (value < 0)
    {
      // Write the negative sign.
      if (buffer < bufend)
        *(buffer++) = '-';

      // Flip the sign.
      value = -value;
    }

    // Handle infinity.
    if (isinf(value))
    {
      return packet_write_string(buffer, bufend, "inf", separator);
    }

    // Handle the integer portion.
    int integer = (int) floorf(value);
    int power = 1000000000;
    while ((power > 1) && (power > integer))
    {
      power /= 10;
    }
    while (power > 1)
    {
      int digit = integer / power;
      integer = integer % power;
      power /= 10;
      if (buffer < bufend)
        *(buffer++) = digit + '0';
    }
    if (buffer < bufend)
      *(buffer++) = integer + '0';

    // Handle the decimal.
    if (buffer < bufend)
      *(buffer++) = '.';

    // Handle the fractional portion.
    int fraction = (int) ((fmodf(value, 1.0f) * 1000000.0f) + 0.5f);
    power = 100000;
    while (power > 1)
    {
      int digit = fraction / power;
      fraction = fraction % power;
      power /= 10;
      if (buffer < bufend)
        *(buffer++) = digit + '0';
    }
    if (buffer < bufend)
      *(buffer++) = fraction + '0';

    // Append the field separator.
    if (buffer < bufend)
      *(buffer++) = separator;

    // Null terminate the buffer.
    *(buffer) = 0;
  }

  return buffer;
}

// Write the signed integer value into the packet buffer.
char *packet_write_int(char *buffer, char *bufend, int32_t value, char separator)
{
  // Sanity check the buffer.
  if (buffer < bufend)
  {
    // Handle the negative.
    if (value < 0)
    {
      // Write the negative sign.
      if (buffer < bufend)
        *(buffer++) = '-';

      // Flip the sign.
      value = -value;
    }

    // Handle the digits.
    int power = 1000000000;
    while ((power > 1) && (power > value))
    {
      power /= 10;
    }
    while (power > 1)
    {
      int digit = value / power;
      value = value % power;
      power /= 10;
      if (buffer < bufend)
        *(buffer++) = digit + '0';
    }
    if (buffer < bufend)
      *(buffer++) = value + '0';

    // Append the field separator.
    if (buffer < bufend)
      *(buffer++) = separator;

    // Null terminate the buffer.
    *(buffer) = 0;
  }

  return buffer;
}

// Write the unsigned integer value into the packet buffer.
char *packet_write_uint(char *buffer, char *bufend, uint32_t value, char separator)
{
  // Sanity check the buffer.
  if (buffer < bufend)
  {
    // Handle the digits.
    uint32_t power = 1000000000;
    while ((power > 1) && (power > value))
    {
      power /= 10;
    }
    while (power > 1)
    {
      uint32_t digit = value / power;
      value = value % power;
      power /= 10;
      if (buffer < bufend)
        *(buffer++) = digit + '0';
    }
    if (buffer < bufend)
      *(buffer++) = value + '0';

    // Append the field separator.
    if (buffer < bufend)
      *(buffer++) = separator;

    // Null terminate the buffer.
    *(buffer) = 0;
  }

  return buffer;
}

// Write the timestamp into the packet buffer.
char *packet_write_timestamp(char *buffer, char *bufend, int64_t timestamp, char separator)
{
  // Sanity check the buffer.
  if (buffer < bufend)
  {
    // Break the timestamp into seconds and nanoseconds.
    int32_t secs = abs((int32_t) (timestamp / 1000000000));
    int32_t nsecs = abs((int32_t) (timestamp % 1000000000));

    // Handle the seconds portion.
    int32_t power = 1000000000;
    while ((power > 1) && (power > secs))
    {
      power /= 10;
    }
    while (power > 1)
    {
      int32_t digit = secs / power;
      secs = secs % power;
      power /= 10;
      if (buffer < bufend)
        *(buffer++) = digit + '0';
    }
    if (buffer < bufend)
      *(buffer++) = secs + '0';

    // Handle the decimal.
    if (buffer < bufend)
      *(buffer++) = '.';

    // Handle the nanoseconds portion.
    power = 100000000;
    while (power > 1)
    {
      int32_t digit = nsecs / power;
      nsecs = nsecs % power;
      power /= 10;
      if (buffer < bufend)
        *(buffer++) = digit + '0';
    }
    if (buffer < bufend)
      *(buffer++) = nsecs + '0';

    // Append the field separator.
    if (buffer < bufend)
      *(buffer++) = separator;

    // Null terminate the buffer.
    *(buffer) = 0;
  }

  return buffer;
}
