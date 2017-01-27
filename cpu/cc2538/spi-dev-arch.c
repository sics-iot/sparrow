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
 * Author: Joakim Eriksson, joakime@sics.se
 *
 */

/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include "dev/spi-dev.h"
#include "spi-dev-arch.h"
#include "spi-arch.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
uint8_t
spi_dev_arch_lock(spi_device_t *dev)
{
  spi_bus_t *bus;
  bus = dev->bus;

  if(bus->lock) {
    return BUS_STATUS_BUS_LOCKED;
  }
  /* Add support for timeout also!!! */
  bus->lock = 1;
  bus->lock_device = dev;

  PRINTF("SPI: lock\n");

  return BUS_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
uint8_t
spi_dev_arch_has_lock(spi_device_t *dev)
{
  return dev->bus->lock_device == dev;
}
/*---------------------------------------------------------------------------*/
uint8_t
spi_dev_arch_is_bus_locked(spi_device_t *dev)
{
  return dev->bus->lock;
}
/*---------------------------------------------------------------------------*/
uint8_t
spi_dev_arch_unlock(spi_device_t *dev)
{
  dev->bus->lock = 0;
  dev->bus->lock_device = NULL;

  PRINTF("SPI: unlock\n");
  return BUS_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
uint8_t
spi_dev_arch_restart_timeout(spi_device_t *dev)
{
  /* do nothing at the moment */
  return BUS_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
/* Assumes that checking dev and bus is not NULL before calling this */
uint8_t
spi_dev_arch_transfer(spi_device_t *dev,
                      const uint8_t *write_buf, int wlen,
                      uint8_t *inbuf, int rlen, int ignore_len)
{
  int i;
  int totlen;
  uint8_t c;
  uint8_t spi_instance;

  spi_instance = dev->bus->config.instance;

  PRINTF("SPI: transfer (r:%d,w:%d) ", rlen, wlen);

  if(write_buf == NULL && wlen > 0) {
    return BUS_STATUS_EINVAL;
  }
  if(inbuf == NULL && rlen > 0) {
    return BUS_STATUS_EINVAL;
  }

  totlen = MAX(rlen + ignore_len, wlen);

  if(totlen == 0) {
    /* Nothing to do */
    return BUS_STATUS_OK;
  }

  PRINTF("%c%c%c: %u ", rlen > 0 ? 'R' : '-', wlen > 0 ? 'W' : '-',
         ignore_len > 0 ? 'S' : '-', totlen);

  for(i = 0; i < totlen; i++) {
    SPIX_WAITFORTxREADY(spi_instance);
    c = i < wlen ? write_buf[i] : 0;
    SPIX_BUF(spi_instance) = c;
    PRINTF("%c%02x->", i < rlen ? ' ' : '#', c);
    SPIX_WAITFOREOTx(spi_instance);
    SPIX_WAITFOREORx(spi_instance);
    c = SPIX_BUF(spi_instance);
    if(i < rlen) {
      inbuf[i] = c;
    }
    PRINTF("%02x", c);
  }

  PRINTF("\n");
  return BUS_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
