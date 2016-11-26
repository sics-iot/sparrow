/*
 * Copyright (c) 2013-2016, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         STTS751 Temperature sensor chip driver
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 */

#include "contiki.h"
#include "sys/uptime.h"
#include "dev/watchdog.h"
#include "dev/i2c.h"
#include "stts751.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifndef STTS751_I2C_ADDRESS
#define STTS751_I2C_ADDRESS STTS751_I2C_CONF_ADDRESS
#endif

static uint8_t stts751_write_register(uint64_t when, uint8_t reg, uint8_t data);
static uint8_t stts751_read_register(uint64_t when, uint8_t reg, uint8_t *data);
static uint8_t stts751_send_byte(uint64_t when, uint8_t reg);
static uint8_t stts751_receive_byte(uint64_t when, uint8_t *data);

/*---------------------------------------------------------------------------*/
uint32_t
stts751_errorval(void)
{
  return STTS751_MILLIKELVIN_ERRORVAL;
}
/*---------------------------------------------------------------------------*/
static uint8_t
stts751_configure(uint64_t when)
{
  uint8_t ld = 0xff;
  int count = 0;
  uint8_t status;

  do {
    watchdog_periodic();
    when = uptime_read();
    status = stts751_read_register(when, STTS751_MANUFACTURER_ID, &ld);
    count++;
  } while((status == STTS751_STATUS_OK) && (ld != 0x53) && (count < 1000));

  if(status != STTS751_STATUS_OK) {
    PRINTF("stts751: (configure) read-ID-err %u\n", (unsigned int)status);
    return status;
  }

  status = stts751_write_register(when, STTS751_CONFIGURATION, 0xcc);
  if(status == STTS751_STATUS_OK) {
    status = stts751_write_register(when, STTS751_CONVERSION_RATE, 0x00);
  } else {
    PRINTF("stts751: (configure) write-config-err %u\n", (unsigned int)status);
    return status;
  }

  status = stts751_read_register(when, STTS751_CONFIGURATION, &ld);
  if(status == STTS751_STATUS_OK) {
    PRINTF("stts751: configuration: 0x%02x\n", ld);
  }

  if(status == STTS751_STATUS_OK) {
    status = stts751_write_register(when, STTS751_SMBUS_TIMEOUT_ENABLE, 0x00);
  } else {
    PRINTF("stts751: (configure) write-smbus-err %u\n", (unsigned int)status);
    return status;
  }
  status = stts751_read_register(when, STTS751_CONFIGURATION, &ld);
  if(status != STTS751_STATUS_OK) {
    PRINTF("stts751: (configure) read-reg-last-err %u\n", status);
    return status;
  } else {
    if(ld != 0xcc) {
      PRINTF("stts751: (configure) config-mismatch-err: 0x%02x\n", ld);
      return STTS751_STATUS_EINVAL;
    }
  }

  return STTS751_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

/*
 * Write one register
 */
static uint8_t
stts751_write_register(uint64_t when, uint8_t reg, uint8_t data)
{
  uint8_t buf[2];
  buf[0] = reg;
  buf[1] = data;
  if(i2c_burst_send(STTS751_I2C_ADDRESS, buf, 2) != I2C_MASTER_ERR_NONE) {
    return STTS751_STATUS_EINVAL;
  }
  return STTS751_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

/*
 * Read one register
 */
static uint8_t
stts751_read_register(uint64_t when, uint8_t reg, uint8_t *data)
{
  if(i2c_single_send(STTS751_I2C_ADDRESS, reg) != I2C_MASTER_ERR_NONE) {
    return STTS751_STATUS_EINVAL;
  }
  if(i2c_single_receive(STTS751_I2C_ADDRESS, data) != I2C_MASTER_ERR_NONE) {
    return STTS751_STATUS_EINVAL;
  }
  return STTS751_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

/*
 * Send byte; set address register.
 */
static uint8_t
stts751_send_byte(uint64_t when, uint8_t reg)
{
  if(i2c_single_send(STTS751_I2C_ADDRESS, reg) != I2C_MASTER_ERR_NONE) {
    return STTS751_STATUS_EINVAL;
  }
  return STTS751_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

/*
 * Receive byte; read register pointed to by address register.
 */
static uint8_t
stts751_receive_byte(uint64_t when, uint8_t *data)
{
  if(i2c_single_receive(STTS751_I2C_ADDRESS, data) != I2C_MASTER_ERR_NONE) {
    return STTS751_STATUS_EINVAL;
  }
  return STTS751_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

/*
 * Convert temperature once and return millikelvin
 *
 * Return STTS751_MILLIKELVIN_ERRORVAL on error.
 */
uint32_t
stts751_millikelvin(void)
{
  uint64_t when;
  uint8_t status;
  uint8_t buf;
  uint8_t poll;
  int32_t temp = 0;

  when = uptime_read();
  status = STTS751_STATUS_OK;

  i2c_init(I2C_SDA_PORT, I2C_SDA_PIN, I2C_SCL_PORT, I2C_SCL_PIN, I2C_SCL_FAST_BUS_SPEED);

  if(stts751_configure(when) != STTS751_STATUS_OK) {
    return STTS751_MILLIKELVIN_ERRORVAL;
  }

  status = stts751_write_register(when, STTS751_ONE_SHOT, 0x42);
  if(status == STTS751_STATUS_OK) {
    status = stts751_send_byte(when, STTS751_STATUS_REG);
  }
  do {
    status = stts751_receive_byte(when, &poll);
  } while((status == STTS751_STATUS_OK) && (poll & STTS751_STATUS_REG_BUSY));

  if(status == STTS751_STATUS_OK) {
    status = stts751_read_register(when, STTS751_TEMPERATURE_VALUE_HIGH_BYTE, &buf);
  }
  if(status == STTS751_STATUS_OK) {
    temp = (int8_t)buf << 8;
    status = stts751_read_register(when, STTS751_TEMPERATURE_VALUE_LOW_BYTE, &buf);
  }

  i2c_master_disable();

  if(status == STTS751_STATUS_OK) {
    temp |= buf;
    /* this here is celsius * 256, convert to celsius * 1000 by multiplying with 125 and dividing with 32 */
    temp *= 125;
    temp /= 32;
    temp += 273150;

    return temp;
  }
  return STTS751_MILLIKELVIN_ERRORVAL;
}
/*---------------------------------------------------------------------------*/
