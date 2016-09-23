/*
 * Copyright (c) 2015, Yanzi Networks AB.
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
 *         A RDC proxy to support both serial radio and standard (node) operation.
 * \author
 *         Ioannis Glaropoulos <ioannisg@kth.se>
 */

#include "net/netstack.h"
#include <string.h>
#include "dev/leds.h"


#ifndef DUAL_MODE_SERIAL_RADIO_RDC_DRIVER
#define DUAL_MODE_SERIAL_RADIO_RDC_DRIVER sniffer_rdc_driver
#endif /* DUAL_MODE_SERIAL_RADIO_RDC_DRIVER */

#ifndef DUAL_MODE_STANDARD_MODE_RDC_DRIVER
#define DUAL_MODE_STANDARD_MODE_RDC_DRIVER nullrdc_driver
#endif /* DUAL_MODE_STANDARD_MODE_RDC_DRIVER */

#ifndef DUAL_MODE_SERIAL_RADIO_FRAMER
#define DUAL_MODE_SERIAL_RADIO_FRAMER no_framer
#endif /* DUAL_MODE_SERIAL_RADIO_FRAMER */

#ifndef DUAL_MODE_STANDARD_MODE_FRAMER
#define DUAL_MODE_STANDARD_MODE_FRAMER framer_802154
#endif /* DUAL_MODE_STANDARD_MODE_FRAMER */

extern const struct rdc_driver DUAL_MODE_SERIAL_RADIO_RDC_DRIVER;
extern const struct rdc_driver DUAL_MODE_STANDARD_MODE_RDC_DRIVER;
extern const struct framer DUAL_MODE_SERIAL_RADIO_FRAMER;
extern const struct framer DUAL_MODE_STANDARD_MODE_FRAMER;

static const struct rdc_driver *current_rdc_driver;
static const struct framer *current_framer;
/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
  current_rdc_driver->input();
}
/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{
  current_rdc_driver->send(sent, ptr);
}
/*---------------------------------------------------------------------------*/
static void
send_list(mac_callback_t sent, void *ptr, struct rdc_buf_list *buf_list)
{
  current_rdc_driver->send_list(sent, ptr, buf_list);
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return current_rdc_driver->on();
}
/*---------------------------------------------------------------------------*/
static int
off(int keep_radio_on)
{
  return current_rdc_driver->off(keep_radio_on);
}
/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  return current_rdc_driver->channel_check_interval();
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  if(dual_mode_get_op_mode() == DUAL_MODE_OP_MODE_STANDARD) {
    DUAL_MODE_STANDARD_MODE_RDC_DRIVER.init();
    current_framer = &(DUAL_MODE_STANDARD_MODE_FRAMER);
    current_rdc_driver = &(DUAL_MODE_STANDARD_MODE_RDC_DRIVER);
  } else {
    DUAL_MODE_SERIAL_RADIO_RDC_DRIVER.init();
    current_framer = &(DUAL_MODE_SERIAL_RADIO_FRAMER);
    current_rdc_driver = &(DUAL_MODE_SERIAL_RADIO_RDC_DRIVER);
  }
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver dual_mode_rdc_driver = {
  "dual mode rdc",
  init,
  send_packet,
  send_list,
  packet_input,
  on,
  off,
  channel_check_interval,
};
/*---------------------------------------------------------------------------*/
static int
hdr_length(void)
{
  return current_framer->length();
}
/*---------------------------------------------------------------------------*/
static int
create(void)
{
  return current_framer->create();
}
/*---------------------------------------------------------------------------*/
static int
parse(void)
{
  return current_framer->parse();
}
/*---------------------------------------------------------------------------*/
const struct framer dual_mode_framer = {
  hdr_length,
  create,
  parse,
};
/*---------------------------------------------------------------------------*/
