#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  BUTTON_1,
  BUTTON_COUNT
};

void buttons_init(void);
bool buttons_get_value(uint32_t button_index);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTONS_H__ */
