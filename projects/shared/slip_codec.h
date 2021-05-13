#ifndef __SLIP_CODEC_H__
#define __SLIP_CODEC_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "cmsis_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

// SLIP encoding characters.
#define SLIP_END             0xC0
#define SLIP_ESC             0xDB
#define SLIP_ESC_END         0xDC
#define SLIP_ESC_ESC         0xDD

typedef struct slip_buffer_s
{
  uint8_t *buffer;
  uint8_t *bufend;
  uint8_t *bufptr;
  uint16_t *crc;
  bool overflow;
} slip_buffer_t;

// Encoder/decoder functions.
void slip_encode_buffer(slip_buffer_t *sbuffer, const uint8_t *buffer, size_t buflen);
void slip_encode_string(slip_buffer_t *sbuffer, const char *str);
void slip_decode_buffer(slip_buffer_t *sbuffer, uint8_t *buffer, size_t buflen);
void slip_decode_string(slip_buffer_t *sbuffer, char *str, size_t max_str_len);

// Inline encoder/decoder helper functions.

// Initialize a slip buffer.
__STATIC_INLINE void slip_init(slip_buffer_t *sbuffer, uint8_t *buffer, size_t buflen, uint16_t *crc)
{
  sbuffer->buffer = buffer;
  sbuffer->bufend = buffer + buflen;
  sbuffer->bufptr = buffer;
  sbuffer->crc = crc;
  sbuffer->overflow = false;
}

// Get the overflow flag.
__STATIC_INLINE bool slip_overflow(slip_buffer_t *sbuffer)
{
  return sbuffer->overflow;
}

// Get the count of the encoded/decoded data.
__STATIC_INLINE size_t slip_count(slip_buffer_t *sbuffer)
{
  return (size_t) (sbuffer->bufptr - sbuffer->buffer);
}

// Append an END flag to the buffer.
__STATIC_INLINE void slip_encode_end(slip_buffer_t *sbuffer)
{
  if (sbuffer->bufptr < sbuffer->bufend)
    *(sbuffer->bufptr++) = SLIP_END;
  else
    sbuffer->overflow = true;
}

// Encode bool as 8-bit data.
__STATIC_INLINE void slip_encode_bool(slip_buffer_t *sbuffer, bool data)
{
  uint8_t data_val = data ? 1 : 0;
  slip_encode_buffer(sbuffer, (const uint8_t *) &data_val, sizeof(int8_t));
}

// Encode signed 8-bit data.
__STATIC_INLINE void slip_encode_int8(slip_buffer_t *sbuffer, int8_t data)
{
  slip_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(int8_t));
}

// Encode signed 16-bit data.
__STATIC_INLINE void slip_encode_int16(slip_buffer_t *sbuffer, int16_t data)
{
  slip_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(int16_t));
}

// Encode signed 32-bit data.
__STATIC_INLINE void slip_encode_int32(slip_buffer_t *sbuffer, int32_t data)
{
  slip_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(int32_t));
}

// Encode signed 64-bit data.
__STATIC_INLINE void slip_encode_int64(slip_buffer_t *sbuffer, int64_t data)
{
  slip_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(int64_t));
}

// Encode unsigned 8-bit data.
__STATIC_INLINE void slip_encode_uint8(slip_buffer_t *sbuffer, uint8_t data)
{
  slip_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(uint8_t));
}

// Encode unsigned 16-bit data.
__STATIC_INLINE void slip_encode_uint16(slip_buffer_t *sbuffer, uint16_t data)
{
  slip_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(uint16_t));
}

// Encode unsigned 32-bit data.
__STATIC_INLINE void slip_encode_uint32(slip_buffer_t *sbuffer, uint32_t data)
{
  slip_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(uint32_t));
}

// Encode unsigned 64-bit data.
__STATIC_INLINE void slip_encode_uint64(slip_buffer_t *sbuffer, uint64_t data)
{
  slip_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(uint64_t));
}

// Encode float data.
__STATIC_INLINE void slip_encode_float(slip_buffer_t *sbuffer, float data)
{
  slip_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(float));
}

// Encode double data.
__STATIC_INLINE void slip_encode_double(slip_buffer_t *sbuffer, double data)
{
  slip_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(double));
}

// Encode CRC value without modifying CRC. Normally done as last encode into a packet.
__STATIC_INLINE void slip_encode_crc(slip_buffer_t *sbuffer, uint16_t crc)
{
  uint16_t *tmp_crc = sbuffer->crc;
  sbuffer->crc = NULL;
  slip_encode_buffer(sbuffer, (const uint8_t *) &crc, sizeof(uint16_t));
  sbuffer->crc = tmp_crc;
}

// Decode bool as 8-bit data.
__STATIC_INLINE void slip_decode_bool(slip_buffer_t *sbuffer, bool *data)
{
  uint8_t data_val;
  slip_decode_buffer(sbuffer, (uint8_t *) &data_val, sizeof(uint8_t));
  *data = data_val ? true : false;
}

// Decode signed 8-bit data.
__STATIC_INLINE void slip_decode_int8(slip_buffer_t *sbuffer, int8_t *data)
{
  slip_decode_buffer(sbuffer, (uint8_t *) data, sizeof(int8_t));
}

// Decode signed 16-bit data.
__STATIC_INLINE void slip_decode_int16(slip_buffer_t *sbuffer, int16_t *data)
{
  slip_decode_buffer(sbuffer, (uint8_t *) data, sizeof(int16_t));
}

// Decode signed 32-bit data.
__STATIC_INLINE void slip_decode_int32(slip_buffer_t *sbuffer, int32_t *data)
{
  slip_decode_buffer(sbuffer, (uint8_t *) data, sizeof(int32_t));
}

// Decode signed 64-bit data.
__STATIC_INLINE void slip_decode_int64(slip_buffer_t *sbuffer, int64_t *data)
{
  slip_decode_buffer(sbuffer, (uint8_t *) data, sizeof(int64_t));
}

// Decode unsigned 8-bit data.
__STATIC_INLINE void slip_decode_uint8(slip_buffer_t *sbuffer, uint8_t *data)
{
  slip_decode_buffer(sbuffer, (uint8_t *) data, sizeof(uint8_t));
}

// Decode unsigned 16-bit data.
__STATIC_INLINE void slip_decode_uint16(slip_buffer_t *sbuffer, uint16_t *data)
{
  slip_decode_buffer(sbuffer, (uint8_t *) data, sizeof(uint16_t));
}

// Decode unsigned 32-bit data.
__STATIC_INLINE void slip_decode_uint32(slip_buffer_t *sbuffer, uint32_t *data)
{
  slip_decode_buffer(sbuffer, (uint8_t *) data, sizeof(uint32_t));
}

// Decode unsigned 64-bit data.
__STATIC_INLINE void slip_decode_uint64(slip_buffer_t *sbuffer, uint64_t *data)
{
  slip_decode_buffer(sbuffer, (uint8_t *) data, sizeof(uint64_t));
}

// Decode float data.
__STATIC_INLINE void slip_decode_float(slip_buffer_t *sbuffer, float *data)
{
  slip_decode_buffer(sbuffer, (uint8_t *) data, sizeof(float));
}

// Decode double data.
__STATIC_INLINE void slip_decode_double(slip_buffer_t *sbuffer, double *data)
{
  slip_decode_buffer(sbuffer, (uint8_t *) data, sizeof(double));
}

// Decode CRC value without modifying CRC. Normally done as last decode from a packet.
__STATIC_INLINE void slip_decode_crc(slip_buffer_t *sbuffer, uint16_t *crc)
{
  uint16_t *tmp_crc = sbuffer->crc;
  sbuffer->crc = NULL;
  slip_decode_buffer(sbuffer, (uint8_t *) crc, sizeof(uint16_t));
  sbuffer->crc = tmp_crc;
}

#ifdef __cplusplus
}
#endif

#endif /* __SLIP_CODEC_H__ */
