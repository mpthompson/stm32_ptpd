#ifndef _HARDTIME_H
#define _HARDTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hardtime_init(void);
int64_t hardtime_get(void);

#ifdef __cplusplus
}
#endif

#endif /* _HARDTIME_H */
