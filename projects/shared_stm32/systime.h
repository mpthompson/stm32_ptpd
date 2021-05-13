#ifndef _SYSTIME_H
#define _SYSTIME_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void systime_init(void);
bool systime_is_valid(void);
int64_t systime_get(void);
void systime_set(int64_t systime);
void systime_adjust(int32_t adjustment);
size_t systime_str(char *buffer, size_t buflen);
size_t systime_log(char *buffer, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* _SYSTIME_H */
