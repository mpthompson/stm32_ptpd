#ifndef __EXTINT_H__
#define __EXTINT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  EXTINT_0 = 0,
  EXTINT_1,
  EXTINT_2,
  EXTINT_3,
  EXTINT_4,
  EXTINT_5,
  EXTINT_6,
  EXTINT_7,
  EXTINT_8,
  EXTINT_9,
  EXTINT_10,
  EXTINT_11,
  EXTINT_12,
  EXTINT_13,
  EXTINT_14,
  EXTINT_15,
  EXTINT_COUNT
};

// Callback function for external interrupt handlers.
typedef void (*extint_func_t)(uint32_t extint_line);
typedef void (*extint_func_with_arg_t)(uint32_t extint_line, void *arg);

void extint_init(void);
void extint_set_callback(uint32_t line, extint_func_t callback);
void extint_set_callback_with_arg(uint32_t line, extint_func_with_arg_t callback, void *arg);

// System configurable functions. Implemented as weak functions.
uint32_t extint_config_preempt_priority(void);

#ifdef __cplusplus
}
#endif

#endif /* __EXTINT_H__ */
