#ifndef I2C_ARCH_H_
#define I2C_ARCH_H_

/* The type-defs for the I2C code */

typedef struct {
  uint8_t sda_port;
  uint8_t sda_pin;
  uint8_t scl_port;
  uint8_t scl_pin;
} i2c_bus_config_t;

#endif /* I2C_ARCH */
