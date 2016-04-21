/*
 * Copyright (c) 2015-2016, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of the copyright holders nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
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
 *         Transmit buffers for serial radio.
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"
#include "serial-radio.h"
#include "net/packetbuf.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define MAX_TX_BUF 16

/* timed transmit buffer with fixed packet size               */
/* NOTE: Assumption is 64 bit timer and 1000 ticks per second */

struct tx_buffer {
  clock_time_t time;
  uint8_t id;
  uint8_t len;
  uint8_t txmits;
  uint8_t packet[PACKETBUF_SIZE];
};

struct tx_buffer buffers[MAX_TX_BUF];

static void update_timer();

static struct ctimer send_timer;

/*---------------------------------------------------------------------------*/
static void
handle_send_timer(void *ptr)
{
  struct tx_buffer *buf;
  buf = (struct tx_buffer *) ptr;

  if(buf != NULL) {
    /* Here is when the packets needs to be transmitted!!! */
    /* Prepare packetbuf */
    packetbuf_copyfrom(buf->packet, buf->len);
    packetbuf_set_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS, buf->txmits);
    /* time to send the packet. */
    serial_radio_send_packet(buf->id);
    PRINTF("TRB: timed packet sent at: tgt:%lu time:%lu %u bytes id:%u\n",
	   (unsigned long)buf->time, (unsigned long)clock_time(), buf->len, buf->id);

    /* done - set length to zero to avoid using this again */
    buf->len = 0;
  }
  update_timer();
}


static void
update_timer()
{
  int i;
  clock_time_t min_time;
  struct tx_buffer *buf;
  buf = NULL;
  min_time = clock_time() + 0x8000; /* highest possible value for delay */
  for(i = 0; i < MAX_TX_BUF; i++) {
    if(buffers[i].len > 0) {
      if(min_time > buffers[i].time) {
	min_time = buffers[i].time;
	buf = &buffers[i];
      }
    }
  }
  if(buf != NULL) {
    int diff;
    diff = min_time - clock_time();
    PRINTF("TXB: setting timer to %d ticks from now\n", diff);
    if(diff < 0) {
      diff = 0;
    }
    ctimer_set(&send_timer, diff, handle_send_timer, buf);
  } else {
    /* Nothing to send */
    PRINTF("TXB: Nothing to send\n");
  }
}

/* This can only be called with packet-buf filled with a packet and
   it's attributes */
int
transmit_buffer_add_packet(clock_time_t time, uint8_t id)
{
  int i;
  for(i = 0; i < MAX_TX_BUF; i++) {
    if(buffers[i].len == 0) {
      /* copy the packet data into the buffer */
      /* what more needs to be stored? */
      buffers[i].len = packetbuf_datalen();
      buffers[i].id = id;
      memcpy(buffers[i].packet, packetbuf_dataptr(), packetbuf_datalen());
      buffers[i].time = time;
      buffers[i].txmits = packetbuf_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS);
      update_timer();
      return 1;
    }
  }
  return 0;
}
