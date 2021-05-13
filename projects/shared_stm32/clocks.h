#ifndef __CLOCKS_H__
#define __CLOCKS_H__

#include <stdint.h>
#include <stdbool.h>
#include "hal_system.h"

#ifdef __cplusplus
extern "C" {
#endif

void clocks_init(void);
uint32_t clocks_timer_base_frequency(TIM_TypeDef *tim);
uint32_t clocks_can_base_frequency(CAN_TypeDef *can);
void clocks_gpio_enable(GPIO_TypeDef *port, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* __CLOCKS_H__ */
