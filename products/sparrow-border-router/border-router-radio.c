/*
 * Copyright (c) 2013-2016, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
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
 */

/**
 * \file
 *         A null radio implementation that forwards packets over slip
 *         instead of radio.
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki-conf.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "packetutils.h"
#include "border-router.h"
#include <string.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static radio_value_t radio_values[PACKETUTILS_RADIO_MAX];
static const void *data_to_send;
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  data_to_send = payload;
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  uint8_t buf[transmit_len + 2];

  if(data_to_send == NULL) {
    return RADIO_TX_ERR;
  }

  buf[0] = '!';
  buf[1] = 's';

  /* Copy packet data */
  memcpy(&buf[2], data_to_send, transmit_len);

  write_to_slip(buf, transmit_len + 2);
  return RADIO_TX_OK;
}
/*---------------------------------------------------------------------------*/
static int
send(const void *payload, unsigned short payload_len)
{
  prepare(payload, payload_len);
  return transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
static int
read(void *buf, unsigned short buf_len)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  /* uint8_t buf[4]; */
  int p;

  p = packetutils_from_radio_param(param);
  if(p < 0) {
    /* Unknown parameter */
    return RADIO_RESULT_NOT_SUPPORTED;
  }

  /* buf[0] = '?'; */
  /* buf[1] = 'V'; */
  /* buf[2] = (p >> 8) & 0xff; */
  /* buf[3] = p & 0xff; */
  /* write_to_slip(buf, 4); */

  if(value) {
    *value = radio_values[p];
  }

  return RADIO_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  uint8_t buf[6];
  int p;

  p = packetutils_from_radio_param(param);
  if(p < 0) {
    /* Unknown parameter */
    return RADIO_RESULT_NOT_SUPPORTED;
  }

  buf[0] = '!';
  buf[1] = 'V';
  buf[2] = (p >> 8) & 0xff;
  buf[3] = p & 0xff;
  buf[4] = (value >> 8) & 0xff;
  buf[5] = value & 0xff;

  write_to_slip(buf, 6);

  radio_values[p] = value;

  return RADIO_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
void
border_router_radio_set_value(radio_param_t param, radio_value_t value)
{
  int p;

  p = packetutils_from_radio_param(param);
  if(p >= 0) {
    radio_values[p] = value;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
const struct radio_driver border_router_radio_driver =
  {
    init,
    prepare,
    transmit,
    send,
    read,
    channel_clear,
    receiving_packet,
    pending_packet,
    on,
    off,
    get_value,
    set_value,
    get_object,
    set_object
  };
/*---------------------------------------------------------------------------*/
