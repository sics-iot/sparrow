#include <stdint.h>
#include "dev/i2c-dev.h"
#include "dev/i2c.h"

static volatile uint8_t is_locked = 0;
/*---------------------------------------------------------------------------*/
uint8_t
i2c_arch_lock(i2c_device_t *dev)
{
  i2c_bus_config_t *conf;

  /* The underlying CC2538 I2C API only allows single access */
  if(is_locked) {
    return BUS_STATUS_BUS_LOCKED;
  }
  is_locked = 1;

  if(dev->speed != I2C_NORMAL_BUS_SPEED &&
     dev->speed != I2C_FAST_BUS_SPEED) {
    return BUS_STATUS_EINVAL;
  }

  conf = &dev->bus->config;
  i2c_init(conf->sda_port, conf->sda_pin, conf->scl_port, conf->scl_pin,
           dev->speed);

  i2c_start_timeout(dev->timeout);
  return BUS_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_arch_unlock(i2c_device_t *dev)
{
  i2c_master_disable();
  i2c_stop_timeout();
  is_locked = 0;
  return BUS_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_arch_restart_timeout(i2c_device_t *dev)
{
  i2c_start_timeout(dev->timeout);
  return BUS_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
static uint8_t
convert_status(uint8_t status)
{
  switch(status) {
  case I2C_MASTER_ERR_NONE:
    return BUS_STATUS_OK;
  case I2C_MASTER_ERR_TIMEOUT:
    return BUS_STATUS_TIMEOUT;
  }
  if(status & I2CM_STAT_ADRACK) {
    return BUS_STATUS_ADDRESS_NACK;
  }
  if(status & I2CM_STAT_DATACK) {
    return BUS_STATUS_ADDRESS_NACK;
  }
  return BUS_STATUS_TIMEOUT;
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_arch_write(i2c_device_t *dev, const uint8_t *data, int len)
{
  uint8_t status;
  /* Convert the device address from 8 bit to 7 bit */
  status = i2c_burst_send(dev->address >> 1, (uint8_t *)data, len);
  /* Translate status into something sensible... */
  return convert_status(status);
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_arch_read(i2c_device_t *dev, uint8_t *data, int len)
{
  uint8_t status;
  /* Convert the device address from 8 bit to 7 bit */
  status = i2c_burst_receive(dev->address >> 1, data, len);
  /* Translate status into the defined status */
  return convert_status(status);
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_arch_stop(i2c_device_t *dev)
{
  return BUS_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
