/*
 * Copyright (c) 2011-2016, Swedish Institute of Computer Science.
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
 *         A null RDC implementation that uses framer for headers and sends
 *         the packets over slip instead of radio.
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/netstack.h"
#include "net/mac/frame802154.h"
#include "packetutils.h"
#include "border-router.h"
#include "border-router-rdc.h"
#include <string.h>
#include "sparrow-oam.h"

#define YLOG_LEVEL YLOG_LEVEL_INFO
#define YLOG_NAME  "br-rdc"
#include "ylog.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

/* from border-router-cmds */
clock_time_t get_sr_time(void);

static uint8_t log_rx = 0;
static uint8_t log_tx = 0;

/* The number of callbacks must fit in 8 bit */
#define MAX_CALLBACKS 255
static uint8_t callback_pos;

/* a structure for calling back when packet data is coming back
   from radio... */
struct tx_callback {
  mac_callback_t cback;
  void *ptr;
  struct packetbuf_attr attrs[PACKETBUF_NUM_ATTRS];
  struct packetbuf_addr addrs[PACKETBUF_NUM_ADDRS];
  struct timer timeout;
  uint16_t len;
};

static struct tx_callback callbacks[MAX_CALLBACKS];
/*---------------------------------------------------------------------------*/
static const char *
get_frame_type(uint16_t type)
{
  switch(type) {
  case FRAME802154_BEACONFRAME:
    return "BEACON";
  case FRAME802154_DATAFRAME:
    return "DATA";
  case FRAME802154_ACKFRAME:
    return "ACK";
  case FRAME802154_CMDFRAME:
    return "CMD";
  case FRAME802154_BEACONREQ:
    return "BEACONREQ";
  default:
    return "-";
  }
}
/*---------------------------------------------------------------------------*/
static const char *
get_tx_status(int status)
{
  switch(status) {
  case MAC_TX_OK:
    return "OK";
  case MAC_TX_COLLISION:
    return "COLLISION";
  case MAC_TX_NOACK:
    return "NOACK";
  case MAC_TX_DEFERRED:
    return "DEFERRED";
  case MAC_TX_ERR:
    return "ERR";
  case MAC_TX_ERR_FATAL:
    return "ERR FATAL";
  default:
    return "-";
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
border_router_rdc_get_logging(void)
{
  uint8_t ret = 0;
  if(log_rx) {
    ret |= BORDER_ROUTER_RDC_LOG_RX;
  }
  if(log_tx) {
    ret |= BORDER_ROUTER_RDC_LOG_TX;
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
void
border_router_rdc_set_logging(uint8_t flags)
{
  log_rx = (flags & BORDER_ROUTER_RDC_LOG_RX) != 0;
  log_tx = (flags & BORDER_ROUTER_RDC_LOG_TX) != 0;
}
/*---------------------------------------------------------------------------*/
void
packet_sent(uint8_t sessionid, uint8_t status, uint8_t tx)
{
  if(sessionid < MAX_CALLBACKS) {
    struct tx_callback *callback;
    callback = &callbacks[sessionid];
    packetbuf_clear();
    packetbuf_attr_copyfrom(callback->attrs, callback->addrs);
    YLOG_DEBUG("callback %u status %u, %u\n", sessionid, status, tx);
    if(callback->len == 0) {
      YLOG_DEBUG("*** callback to unused entry %u\n", sessionid);
    }
    if(log_tx) {
      YLOG_PRINT("[TX %3d] %-9s ",
                 callback->len,
                 get_frame_type(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)));
      net_debug_lladdr_print((uip_lladdr_t *)packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
      PRINTA(" %3d tx   %s\n", tx, get_tx_status(status));
    }
    callback->len = 0;
    mac_call_sent_callback(callback->cback, callback->ptr, status, tx);
  } else {
    YLOG_DEBUG("*** ERROR: too high session id %d\n", sessionid);
  }
}
/*---------------------------------------------------------------------------*/
static int
setup_callback(mac_callback_t sent, void *ptr, uint8_t *send_id)
{
  struct tx_callback *callback;
  callback = &callbacks[callback_pos];
  if(callback->len > 0) {
    if(!timer_expired(&callback->timeout)) {
      YLOG_ERROR("**** overflow - no respons to previous callback %u\n", callback_pos);
      return 0;
    }
    YLOG_DEBUG("**** no respons to previous callback %u\n", callback_pos);
  }
  callback->cback = sent;
  callback->ptr = ptr;
  callback->len = packetbuf_totlen();
  timer_set(&callback->timeout, CLOCK_SECOND);
  packetbuf_attr_copyto(callback->attrs, callback->addrs);

  *send_id = callback_pos;
  callback_pos++;
  if(callback_pos >= MAX_CALLBACKS) {
    callback_pos = 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static unsigned long txcount;
static void
send_packet(mac_callback_t sent, void *ptr)
{
  int size;
  /* 3 bytes per packet attribute is required for serialization */
  uint8_t buf[PACKETBUF_NUM_ATTRS * 3 + PACKETBUF_SIZE + 3];
  uint8_t sid;

  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);

  /* ack or not ? */
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_ACK, 1);

  if(NETSTACK_FRAMER.create() < 0) {
    /* Failed to allocate space for headers */
    YLOG_DEBUG("send failed, too large header\n");
    mac_call_sent_callback(sent, ptr, MAC_TX_ERR_FATAL, 0);

  } else {
    char type;

    /* here we send the data over SLIP to the radio-chip */
    type = 'S'; /* default for sending down to SR */

    size = packetutils_serialize_atts(&buf[3], sizeof(buf) - 3);
    if(size < 0 || size + packetbuf_totlen() + 3 > sizeof(buf)) {
      YLOG_DEBUG("send failed, too large header\n");
      mac_call_sent_callback(sent, ptr, MAC_TX_ERR_FATAL, 0);

    } else if(!setup_callback(sent, ptr, &sid)) {
      /* Failed to allocate a new callback for the transmission. The
         packet can not be sent at this time. */
      mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);

    } else {
      buf[0] = '!';
      buf[1] = type;
      buf[2] = sid; /* sequence or session number for this packet */

      /* Copy packet data */
      memcpy(&buf[3 + size], packetbuf_hdrptr(), packetbuf_totlen());

      txcount++;
      YLOG_DEBUG("sent %u/%u bytes with %c session %u (total %lu)\n",
                 packetbuf_totlen(), packetbuf_totlen() + size + 3,
                 type, sid, txcount);
      write_to_slip(buf, packetbuf_totlen() + size + 3);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
send_list(mac_callback_t sent, void *ptr, struct rdc_buf_list *buf_list)
{
  if(buf_list != NULL) {
    queuebuf_to_packetbuf(buf_list->buf);
    send_packet(sent, ptr);
  }
}
/*---------------------------------------------------------------------------*/
static int drop_all = 0;
int border_router_rdc_dropped;
static void
packet_input(void)
{
  int ret;

  if(drop_all) {
    /* Drop all packets */
    border_router_rdc_dropped++;
    return;
  }

  ret = NETSTACK_FRAMER.parse();
  if(ret == FRAMER_FRAME_HANDLED) {
    /* Packet has already been handled by the framer */
  } else if(ret < 0) {
    YLOG_DEBUG("failed to parse frame %d (%u bytes)\n",
               ret, packetbuf_datalen());
  } else {
    YLOG_DEBUG("RECV %u\n", packetbuf_datalen());

    if(log_rx) {
      YLOG_PRINT("[RX %3d] %-9s %u ",
             packetbuf_datalen(),
             get_frame_type(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)),
             packetbuf_attr(PACKETBUF_ATTR_TIMESTAMP));
      net_debug_lladdr_print((uip_lladdr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER));
      PRINTA(" %d dBm\n",
             (int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
    }

    NETSTACK_MAC.input();
  }
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  drop_all = 0;
  border_router_rdc_dropped = 0;
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
off(int keep_radio_on)
{
  drop_all = keep_radio_on ? 0 : 1;
  return 1;
}
/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  callback_pos = 0;
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver border_router_rdc_driver = {
  "br-rdc",
  init,
  send_packet,
  send_list,
  packet_input,
  on,
  off,
  channel_check_interval,
};
/*---------------------------------------------------------------------------*/
