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
 *         A RDC proxy to support packet sniffing
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "net/netstack.h"
#include "serial-radio.h"
#include "sniffer-rdc.h"

#ifdef SNIFFER_RDC_CONF_DRIVER
#define SNIFFER_RDC SNIFFER_RDC_CONF_DRIVER
#else
#define SNIFFER_RDC nullrdc_driver
#endif

extern const struct rdc_driver SNIFFER_RDC;

static sniffer_rdc_callback_t sniff_input;
/*---------------------------------------------------------------------------*/
int
sniffer_rdc_is_sniffing(void)
{
  return serial_radio_mode == RADIO_MODE_SNIFFER && sniff_input != NULL;
}
/*---------------------------------------------------------------------------*/
void
sniffer_rdc_set_sniffer(sniffer_rdc_callback_t input)
{
  sniff_input = input;
}
/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
  switch(serial_radio_mode) {
  case RADIO_MODE_NORMAL:
    SNIFFER_RDC.input();
    break;
  case RADIO_MODE_SNIFFER:
    if(sniff_input) {
      switch(sniff_input()) {
      case SNIFFER_RDC_FILTER_DROP:
        break;
      case SNIFFER_RDC_FILTER_PROCESS:
        NETSTACK_NETWORK.input();
        break;
      default:
        /* Process as default */
        NETSTACK_NETWORK.input();
        break;
      }
    }
    break;
  case RADIO_MODE_IDLE:
  case RADIO_MODE_SCAN:
    break;
  default:
    /* Ignore incoming packets in all other modes */
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{
  switch(serial_radio_mode) {
  case RADIO_MODE_IDLE:
    /* Ignore packet transmissions in idle mode */
    mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
    break;
  case RADIO_MODE_NORMAL:
    SNIFFER_RDC.send(sent, ptr);
    break;
  case RADIO_MODE_SNIFFER:
    mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
    break;
  case RADIO_MODE_SCAN:
    /* Ignore packet transmissions in scan mode */
    mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void
send_list(mac_callback_t sent, void *ptr, struct rdc_buf_list *buf_list)
{
  switch(serial_radio_mode) {
  case RADIO_MODE_IDLE:
    /* Ignore packet transmissions in idle mode */
    mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
    break;
  case RADIO_MODE_NORMAL:
    SNIFFER_RDC.send_list(sent, ptr, buf_list);
    break;
  case RADIO_MODE_SNIFFER:
    mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
    break;
  case RADIO_MODE_SCAN:
    /* Ignore packet transmissions in scan mode */
    break;
  }
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return SNIFFER_RDC.on();
}
/*---------------------------------------------------------------------------*/
static int
off(int keep_radio_on)
{
  return SNIFFER_RDC.off(keep_radio_on);
}
/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  return SNIFFER_RDC.channel_check_interval();
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  SNIFFER_RDC.init();
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver sniffer_rdc_driver = {
  "snifferrdc",
  init,
  send_packet,
  send_list,
  packet_input,
  on,
  off,
  channel_check_interval,
};
/*---------------------------------------------------------------------------*/
