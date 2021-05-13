#ifndef __NBO_CODEC_H__
#define __NBO_CODEC_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "cmsis_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nbo_buffer_s
{
  uint8_t *buffer;
  uint8_t *bufend;
  uint8_t *bufptr;
  uint16_t *crc;
  bool overflow;
} nbo_buffer_t;

// Encoder/decoder functions.
void nbo_encode_buffer(nbo_buffer_t *sbuffer, const uint8_t *buffer, size_t buflen);
void nbo_encode_string(nbo_buffer_t *sbuffer, const char *str);
void nbo_decode_buffer(nbo_buffer_t *sbuffer, uint8_t *buffer, size_t buflen);
void nbo_decode_string(nbo_buffer_t *sbuffer, char *str, size_t max_str_len);

// Inline encoder/decoder helper functions.

// Initialize a slip buffer.
__STATIC_INLINE void nbo_init(nbo_buffer_t *sbuffer, uint8_t *buffer, size_t buflen, uint16_t *crc)
{
  // Reset the CRC.
  if (crc) *crc = 0;

  // Initialize the buffer.
  sbuffer->buffer = buffer;
  sbuffer->bufend = buffer + buflen;
  sbuffer->bufptr = buffer;
  sbuffer->crc = crc;
  sbuffer->overflow = false;
}

// Get the overflow flag.
__STATIC_INLINE bool nbo_overflow(nbo_buffer_t *sbuffer)
{
  return sbuffer->overflow;
}

// Get the count of the encoded/decoded data.
__STATIC_INLINE size_t nbo_count(nbo_buffer_t *sbuffer)
{
  return (size_t) (sbuffer->bufptr - sbuffer->buffer);
}

// Encode bool as 8-bit data.
__STATIC_INLINE void nbo_encode_bool(nbo_buffer_t *sbuffer, bool data)
{
  uint8_t data_val = data ? 1 : 0;
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data_val, sizeof(int8_t));
}

// Encode signed 8-bit data.
__STATIC_INLINE void nbo_encode_int8(nbo_buffer_t *sbuffer, int8_t data)
{
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(int8_t));
}

// Encode signed 16-bit data.
__STATIC_INLINE void nbo_encode_int16(nbo_buffer_t *sbuffer, int16_t data)
{
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(int16_t));
}

// Encode signed 32-bit data.
__STATIC_INLINE void nbo_encode_int32(nbo_buffer_t *sbuffer, int32_t data)
{
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(int32_t));
}

// Encode signed 64-bit data.
__STATIC_INLINE void nbo_encode_int64(nbo_buffer_t *sbuffer, int64_t data)
{
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(int64_t));
}

// Encode unsigned 8-bit data.
__STATIC_INLINE void nbo_encode_uint8(nbo_buffer_t *sbuffer, uint8_t data)
{
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(uint8_t));
}

// Encode unsigned 16-bit data.
__STATIC_INLINE void nbo_encode_uint16(nbo_buffer_t *sbuffer, uint16_t data)
{
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(uint16_t));
}

// Encode unsigned 32-bit data.
__STATIC_INLINE void nbo_encode_uint32(nbo_buffer_t *sbuffer, uint32_t data)
{
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(uint32_t));
}

// Encode unsigned 64-bit data.
__STATIC_INLINE void nbo_encode_uint64(nbo_buffer_t *sbuffer, uint64_t data)
{
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(uint64_t));
}

// Encode float data.
__STATIC_INLINE void nbo_encode_float(nbo_buffer_t *sbuffer, float data)
{
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(float));
}

// Encode double data.
__STATIC_INLINE void nbo_encode_double(nbo_buffer_t *sbuffer, double data)
{
  nbo_encode_buffer(sbuffer, (const uint8_t *) &data, sizeof(double));
}

// Encode CRC value without modifying CRC. Normally done as last encode into a packet.
__STATIC_INLINE void nbo_encode_crc(nbo_buffer_t *sbuffer, uint16_t crc)
{
  uint16_t *tmp_crc = sbuffer->crc;
  sbuffer->crc = NULL;
  nbo_encode_buffer(sbuffer, (const uint8_t *) &crc, sizeof(uint16_t));
  sbuffer->crc = tmp_crc;
}

// Decode bool as 8-bit data.
__STATIC_INLINE void nbo_decode_bool(nbo_buffer_t *sbuffer, bool *data)
{
  uint8_t data_val;
  nbo_decode_buffer(sbuffer, (uint8_t *) &data_val, sizeof(uint8_t));
  *data = data_val ? true : false;
}

// Decode signed 8-bit data.
__STATIC_INLINE void nbo_decode_int8(nbo_buffer_t *sbuffer, int8_t *data)
{
  nbo_decode_buffer(sbuffer, (uint8_t *) data, sizeof(int8_t));
}

// Decode signed 16-bit data.
__STATIC_INLINE void nbo_decode_int16(nbo_buffer_t *sbuffer, int16_t *data)
{
  nbo_decode_buffer(sbuffer, (uint8_t *) data, sizeof(int16_t));
}

// Decode signed 32-bit data.
__STATIC_INLINE void nbo_decode_int32(nbo_buffer_t *sbuffer, int32_t *data)
{
  nbo_decode_buffer(sbuffer, (uint8_t *) data, sizeof(int32_t));
}

// Decode signed 64-bit data.
__STATIC_INLINE void nbo_decode_int64(nbo_buffer_t *sbuffer, int64_t *data)
{
  nbo_decode_buffer(sbuffer, (uint8_t *) data, sizeof(int64_t));
}

// Decode unsigned 8-bit data.
__STATIC_INLINE void nbo_decode_uint8(nbo_buffer_t *sbuffer, uint8_t *data)
{
  nbo_decode_buffer(sbuffer, (uint8_t *) data, sizeof(uint8_t));
}

// Decode unsigned 16-bit data.
__STATIC_INLINE void nbo_decode_uint16(nbo_buffer_t *sbuffer, uint16_t *data)
{
  nbo_decode_buffer(sbuffer, (uint8_t *) data, sizeof(uint16_t));
}

// Decode unsigned 32-bit data.
__STATIC_INLINE void nbo_decode_uint32(nbo_buffer_t *sbuffer, uint32_t *data)
{
  nbo_decode_buffer(sbuffer, (uint8_t *) data, sizeof(uint32_t));
}

// Decode unsigned 64-bit data.
__STATIC_INLINE void nbo_decode_uint64(nbo_buffer_t *sbuffer, uint64_t *data)
{
  nbo_decode_buffer(sbuffer, (uint8_t *) data, sizeof(uint64_t));
}

// Decode float data.
__STATIC_INLINE void nbo_decode_float(nbo_buffer_t *sbuffer, float *data)
{
  nbo_decode_buffer(sbuffer, (uint8_t *) data, sizeof(float));
}

// Decode double data.
__STATIC_INLINE void nbo_decode_double(nbo_buffer_t *sbuffer, double *data)
{
  nbo_decode_buffer(sbuffer, (uint8_t *) data, sizeof(double));
}

// Decode CRC value without modifying CRC. Normally done as last decode from a packet.
__STATIC_INLINE void nbo_decode_crc(nbo_buffer_t *sbuffer, uint16_t *crc)
{
  uint16_t *tmp_crc = sbuffer->crc;
  sbuffer->crc = NULL;
  nbo_decode_buffer(sbuffer, (uint8_t *) crc, sizeof(uint16_t));
  sbuffer->crc = tmp_crc;
}

#ifdef __cplusplus
}
#endif

#endif /* __NBO_CODEC_H__ */
