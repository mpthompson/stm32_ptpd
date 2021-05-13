#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int packet_parse_args(char *packet, char *argv[], int argv_len, char delimiter);
int64_t packet_parse_timestamp(char *arg);

char *packet_write_char(char *buffer, char *bufend, char ch, char separator);
char *packet_write_string(char *buffer, char *bufend, char *str, char separator);
char *packet_write_float(char *buffer, char *bufend, float value, char separator);
char *packet_write_int(char *buffer, char *bufend, int32_t value, char separator);
char *packet_write_uint(char *buffer, char *bufend, uint32_t value, char separator);
char *packet_write_timestamp(char *buffer, char *bufend, int64_t timestamp, char separator);

#ifdef __cplusplus
}
#endif

#endif /* __PACKET_H__ */
