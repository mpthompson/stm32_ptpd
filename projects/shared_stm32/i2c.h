#ifndef __I2C_H__
#define __I2C_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void i2c_init(void);
int i2c_register_write(uint8_t dev_address, uint8_t reg_address, uint8_t *buffer, uint16_t buflen);
int i2c_register_read(uint8_t dev_address, uint8_t reg_address, uint8_t *buffer, uint16_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */
