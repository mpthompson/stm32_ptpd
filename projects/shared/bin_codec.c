#include <stdlib.h>
#include <string.h>
#include "crc.h"
#include "bin_codec.h"

// Binary encode the data buffer.
void bin_encode_buffer(bin_buffer_t *sbuffer, const uint8_t *buffer, size_t buflen)
{
  const uint8_t *src = buffer;
  const uint8_t *src_end = buffer + buflen;

  // Update the CRC on the unencoded data.
  if (sbuffer->crc) *(sbuffer->crc) = crc16_process(*(sbuffer->crc), buffer, (uint32_t) buflen);

  // Loop over the data to be encoded.
  while (src < src_end)
  {
    // Get the next character to process.
    uint8_t ch = *(src++);

    // Make sure we haven't overflowed.
    if (sbuffer->bufptr < sbuffer->bufend)
    {
      // Encode the character.
      *(sbuffer->bufptr++) = ch;
    }
    else
    {
      // Set the overflow flag.
      sbuffer->overflow = true;
    }
  }
}

// Binary encode the string buffer.
void bin_encode_string(bin_buffer_t *sbuffer, const char *str)
{
  const uint8_t *src = (uint8_t *) str;
  const uint8_t *src_end = (uint8_t *) str + strlen(str) + 1;

  // Update the CRC on the unencoded data.
  if (sbuffer->crc) *(sbuffer->crc) = crc16_process(*(sbuffer->crc), (uint8_t *) str, (uint32_t) strlen(str) + 1);

  // Loop over the data to be encoded.
  while (src < src_end)
  {
    // Get the next character to process.
    uint8_t ch = *(src++);

    // Make sure we haven't overflowed.
    if (sbuffer->bufptr < sbuffer->bufend)
    {
      // Encode the character.
      *(sbuffer->bufptr++) = ch;
    }
    else
    {
      // Set the overflow flag.
      sbuffer->overflow = true;
    }
  }
}

// Binary decode the data buffer.
void bin_decode_buffer(bin_buffer_t *sbuffer, uint8_t *buffer, size_t buflen)
{
  uint8_t *dst = buffer;
  uint8_t *dst_end = buffer + buflen;

  // Loop over the data to be decoded.
  while (dst < dst_end)
  {
    uint8_t ch = 0;

    // Make sure we haven't overflowed.
    if (sbuffer->bufptr < sbuffer->bufend)
    {
      // Get the next character to process.
      ch = *(sbuffer->bufptr++);
    }
    else
    {
      // Set the overflow flag.
      sbuffer->overflow = true;
    }

    // Insert the character into the destination buffer.
    *(dst++) = ch;
  }

  // Update the CRC on the unencoded data.
  if (sbuffer->crc) *(sbuffer->crc) = crc16_process(*(sbuffer->crc), buffer, (uint32_t) buflen);
}

// Binary decode the string buffer.
void bin_decode_string(bin_buffer_t *sbuffer, char *str, size_t max_str_len)
{
  uint8_t *dst = (uint8_t *) str;
  uint8_t *dst_end = (uint8_t *) str + max_str_len - 1;

  // Sanity check the length of the string buffer.
  if (max_str_len < 1) return;

  // Initialize the stop variable.
  bool stop = false;

  // Loop over the data to be decoded.
  while (!stop)
  {
    uint8_t ch = 0;

    // Make sure we haven't overflowed.
    if (sbuffer->bufptr < sbuffer->bufend)
    {
      // Get the next character to process.
      ch = *(sbuffer->bufptr++);
    }
    else
    {
      // Set the overflow flag.
      sbuffer->overflow = true;
    }

    // Insert the character into the destination buffer.
    if (dst < dst_end) *(dst++) = (char) ch;

    // Stop when a null character is reached or the end buffer is reached.
    stop = (ch == 0) || (dst >= dst_end);
  }

  // Make sure the buffer is null terminated.
  *dst = 0;

  // Update the CRC on the unencoded data.
  if (sbuffer->crc) *(sbuffer->crc) = crc16_process(*(sbuffer->crc), (uint8_t *) str, (uint32_t) strlen(str) + 1);
}
