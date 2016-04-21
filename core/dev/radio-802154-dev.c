/*
 * Copyright (c) 2016, Yanzi Networks AB.
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
 *
 */

/**
 * \file
 *         Support functions for IEEE 802.15.4 radios
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"
#include "net/mac/frame802154.h"
#include "dev/radio-802154-dev.h"
#include "net/packetbuf.h"
#include "dev/radio.h"
#include "dev/watchdog.h"

/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
int
radio_802154_rx_buf_is_empty(const struct radio_802154_driver *radio)
{
  return radio->state->read_buf == radio->state->write_buf;
}
/*---------------------------------------------------------------------------*/
int
radio_802154_rx_buf_has_empty(const struct radio_802154_driver *radio)
{
  return (radio->state->write_buf + 1) % RADIO_802154_BUFFER_COUNT != radio->state->read_buf;
}
/*---------------------------------------------------------------------------*/
radio_802154_rf_buffer_t *
radio_802154_get_writebuf(const struct radio_802154_driver *radio)
{
  if(radio_802154_rx_buf_has_empty(radio)) {
    radio_802154_rf_buffer_t *buffer;
    buffer = &radio->state->rx_bufs[radio->state->write_buf];
    return buffer;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
radio_802154_release_writebuf(const struct radio_802154_driver *radio, uint8_t packet_ok)
{
  /* This should not happen... since it was already allocated */
  if(!radio_802154_rx_buf_has_empty(radio)) {
    return;
  }
  if(packet_ok) {
    /* move the write buffer index only if the packet in it is ok */
    radio->state->write_buf = (radio->state->write_buf + 1) % RADIO_802154_BUFFER_COUNT;
    PRINTF("Released write buffer - write: %d read: %d\n",
           radio->state->write_buf, radio->state->read_buf);
  }
}

/*---------------------------------------------------------------------------*/
radio_802154_rf_buffer_t *
radio_802154_get_readbuf(const struct radio_802154_driver *radio)
{
  if(radio_802154_rx_buf_is_empty(radio)) {
    return NULL;
  }
  return &radio->state->rx_bufs[radio->state->read_buf];
}
/*---------------------------------------------------------------------------*/
void
radio_802154_release_readbuf(const struct radio_802154_driver *radio)
{
  if(radio_802154_rx_buf_is_empty(radio)) {
    return;
  }
  radio->state->read_buf = (radio->state->read_buf + 1) % RADIO_802154_BUFFER_COUNT;
  PRINTF("Released read buffer - write: %d read: %d\n",
         radio->state->write_buf, radio->state->read_buf);
}
/*---------------------------------------------------------------------------*/
/* called from receive Interrupt */
radio_802154_rf_buffer_t *
radio_802154_handle_ack(const struct radio_802154_driver *radio,
                        const uint8_t *buf, int len)
{
  radio_802154_rf_buffer_t *rf_buf;

  /* handle a potential ACK in a small packet */
  if(len < RADIO_802154_ACK_LEN) {
    /* Ignore and do not store... */
    return NULL;
  }
  /* Check if we have an ACK packet */
  if((buf[0] & 7) == FRAME802154_ACKFRAME) {
    if(buf[2] == radio->state->last_out_seq &&
       radio->state->last_tx_no > 0) {
      radio->state->acked = 1;
    }

    /* Filter acks when in auto ack mode */
    if((radio->state->rx_mode & RADIO_RX_MODE_AUTOACK) != 0) {
      return NULL;
    }
  }

  /* Need to store the packet - could be something else or in sniffer mode */
  rf_buf = radio_802154_get_writebuf(radio);
  if(rf_buf != NULL) {
    memcpy(rf_buf->buffer, buf, len);
    rf_buf->len = len;
    return rf_buf;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/* Send a packet and if needed wait for the ack. - The radio driver is assumed
   to provide a already awake radio that is ready to transmit */
int
radio_802154_transmit(const struct radio_802154_driver *radio, unsigned short total_len)
{
  uint8_t dsn;
  int is_broadcast = 0;
  int ret = RADIO_TX_ERR;
  uint8_t max_tx_attempts = 0;
  uint8_t attempts = 0;
  uint8_t collisions = 0;
  rtimer_clock_t wt;

  /* read out the sequence number */
  dsn = ((uint8_t *)packetbuf_hdrptr())[2] & 0xff;
  radio->state->last_tx_no = 0;
  radio->state->last_out_seq = dsn;
  radio->state->acked = 0;
  is_broadcast = packetbuf_holds_broadcast();

  if(packetbuf_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS) > 0) {
    max_tx_attempts = packetbuf_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS);
  } else {
    /* TODO: Default 5 for now... */
    max_tx_attempts = 5;
  }

  PRINTF("Sending packet: seq: %d BC: %d attemts: %d len:%d\n", dsn, is_broadcast, max_tx_attempts, total_len);

  while(attempts < max_tx_attempts) {
    /* TODO: - here we should also have some kind of timeout so that we do
       not get stuck for ever... */
    switch(ret = radio->transmit_packet(total_len)) {
    case RADIO_TX_OK:
      /* We did a transmission */
      radio->state->last_tx_no++;
      attempts++;
      if(is_broadcast) {
        /* radio is done - no retransmissions */
        return RADIO_TX_OK;
      } else {
        /* A TX OK on unicast */
        /* Check for ack */
        PRINTF("Unicast %d - checking for ACK:[", radio->state->last_tx_no);
        watchdog_periodic();
        ret = RADIO_TX_NOACK;
        wt = RTIMER_NOW();
        while(RTIMER_CLOCK_LT(RTIMER_NOW(), wt + radio->ack_detect_time)) {
          if(radio->state->acked) {
            /* Packet acked! */
            PRINTF("Returning TX OK (1)\n");
            return RADIO_TX_OK;
          }
          PRINTF("1");
        }
        PRINTF("][");
        if(radio->driver->receiving_packet() ||
           radio->driver->pending_packet() ||
           radio->driver->channel_clear() == 0) {
          watchdog_periodic();
          wt = RTIMER_NOW();
          while(RTIMER_CLOCK_LT(RTIMER_NOW(), wt + radio->ack_receive_time)) {
            if(radio->state->acked) {
              /* Packet acked! */
              PRINTF("Returning TX OK (2)\n");
              return RADIO_TX_OK;
            }
            PRINTF("2");
          }
        }
        if(radio->state->acked) {
          PRINTF("Returning TX OK (3)\n");
          return RADIO_TX_OK;
        }
        PRINTF("] Packet sent - no ACK?\n");
      }
      break;
    case RADIO_TX_COLLISION:
      /* radio did give up... no transmission */
      wt = RTIMER_NOW();
      watchdog_periodic();
      while(RTIMER_CLOCK_LT(RTIMER_NOW(),
                            wt + (radio->collision_wait_time << collisions))) {
      }
      collisions++;
      if(collisions > 4) {
        /* Ok give up this transmission - and continue */
        collisions = 0;
        attempts++; /* count 4 fails as an attempt - but not as transmission */
      }
      PRINTF("Collission: %d %d\n", collisions, attempts);
      ret = RADIO_TX_COLLISION;
      break;
    case RADIO_TX_ERR:
      return RADIO_TX_ERR;
    default:
      return RADIO_TX_ERR;
    }
    /* Here we need to have a short packet gap - not long... */
    wt = RTIMER_NOW();
    while(RTIMER_CLOCK_LT(RTIMER_NOW(), wt + radio->retransmit_wait_time)) {
    }

  }
  return ret;
}
