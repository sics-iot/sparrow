/*
 * Copyright (c) 2016-2016, Yanzi Networks
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COMPANY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 */

#ifndef SPI_DEV_H_
#define SPI_DEV_H_

#include "contiki-conf.h"
#include "dev/bus-status.h"

#ifdef PLATFORM_HAS_SPI_DEV_ARCH
#include "spi-dev-arch.h"
#endif

/* Timer / timing */
typedef struct spi_bus {
  struct spi_device *lock_device;
  volatile uint8_t lock; /* for locking the bus */
#ifdef PLATFORM_HAS_SPI_DEV_ARCH
  spi_bus_config_t config;
#endif
} spi_bus_t;

/* Timing... */
typedef struct spi_device {
  struct spi_bus *bus;
  uint16_t timeout; /* timeout in milliseconds for this device */
  uint8_t (*chip_select)(int on); /* chip-select for this SPI chip - 1 = CS */
#ifdef PLATFORM_HAS_SPI_DEV_ARCH
  spi_device_config_t config;
#endif
} spi_device_t;

/* call for all spi devices */
uint8_t spi_dev_acquire(spi_device_t *dev);
uint8_t spi_dev_release(spi_device_t *dev);
uint8_t spi_dev_restart_timeout(spi_device_t *dev);
int     spi_dev_has_bus(spi_device_t *dev);
uint8_t spi_dev_write_byte(spi_device_t *dev, uint8_t data);
uint8_t spi_dev_read_byte(spi_device_t *dev, uint8_t *data);
uint8_t spi_dev_write(spi_device_t *dev, const uint8_t *data, int size);
uint8_t spi_dev_read(spi_device_t *dev, uint8_t *data, int size);
uint8_t spi_dev_read_skip(spi_device_t *dev, int size);
uint8_t spi_dev_transfer(spi_device_t *dev, const uint8_t *data, int wsize, uint8_t *buf, int rsize, int ignore);
uint8_t spi_dev_chip_select(spi_device_t *dev, uint8_t on);
uint8_t spi_dev_strobe(spi_device_t *dev, uint8_t strobe, uint8_t *status);
uint8_t spi_dev_read_register(spi_device_t *dev, uint8_t reg, uint8_t *data, int size);

/* Arch functions needed per CPU */
uint8_t spi_dev_arch_lock(spi_device_t *dev);
uint8_t spi_dev_arch_unlock(spi_device_t *dev);
uint8_t spi_dev_arch_has_lock(spi_device_t *dev);
uint8_t spi_dev_arch_is_bus_locked(spi_device_t *dev);

/* Initialize the spi bus */
uint8_t spi_dev_arch_init(spi_device_t *dev);
uint8_t spi_dev_arch_transfer(spi_device_t *dev, const uint8_t *data, int wlen, uint8_t *buf, int rlen, int ignore_len);
uint8_t spi_dev_arch_restart_timeout(spi_device_t *dev);


#endif /* SPI_DEV_H */
