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

#ifndef RADIO_802154_DEV_H_
#define RADIO_802154_DEV_H_

#include "contiki-conf.h"

#ifdef RADIO_802154_CONF_WITH_TIMESTAMP
#define RADIO_802154_WITH_TIMESTAMP RADIO_802154_CONF_WITH_TIMESTAMP
#else /* RADIO_802154_WITH_TIMESTAMP */
#define RADIO_802154_WITH_TIMESTAMP 0
#endif /* RADIO_802154_WITH_TIMESTAMP */

#define RADIO_802154_BUFFER_COUNT   8
#define RADIO_802154_BUFFER_SIZE    128
#define RADIO_802154_ACK_LEN        3

#define RADIO_802154_DRIVER(name, driver, ack_detect, ack_receive, collision, retransmit, transmit_packet) \
  static struct radio_802154_state name##_state;                        \
  static const struct radio_802154_driver name = {                      \
    ack_detect, ack_receive, collision, retransmit,                     \
    &name##_state, &driver, transmit_packet                             \
  }

typedef struct radio_802154_rf_buffer {
  int8_t rssi;
  uint8_t crc_corr;
  uint8_t len;
#if RADIO_802154_WITH_TIMESTAMP
  uint16_t timestamp;
#endif
  uint8_t buffer[RADIO_802154_BUFFER_SIZE];
} radio_802154_rf_buffer_t;

struct radio_802154_state {
  volatile uint8_t read_buf;
  volatile uint8_t write_buf;
  volatile uint8_t acked;
  uint8_t last_tx_no;
  uint8_t last_out_seq;
  uint8_t rx_mode;
  uint8_t tx_mode;

  radio_802154_rf_buffer_t rx_bufs[RADIO_802154_BUFFER_COUNT];
};


struct radio_802154_driver {
  /* Configuration */
  uint32_t ack_detect_time;
  uint32_t ack_receive_time;
  uint32_t collision_wait_time;
  uint32_t retransmit_wait_time;

  /* State and functions */
  struct radio_802154_state *state;
  const struct radio_driver *driver;
  /** Send the packet that has previously been prepared. */
  int (* transmit_packet)(unsigned short transmit_len);
};


/* Check if there are anything in buffers */
int radio_802154_rx_buf_is_empty(const struct radio_802154_driver *radio);
/* Check if there are any free buffers */
int radio_802154_rx_buf_has_empty(const struct radio_802154_driver *radio);

/* To be called from ISR to get a buffer to write to. */
radio_802154_rf_buffer_t *radio_802154_get_writebuf(const struct radio_802154_driver *radio);
void radio_802154_release_writebuf(const struct radio_802154_driver *radio, uint8_t packet_ok);

radio_802154_rf_buffer_t *radio_802154_get_readbuf(const struct radio_802154_driver *radio);
void radio_802154_release_readbuf(const struct radio_802154_driver *radio);

void radio_802154_clear(const struct radio_802154_driver *radio);

/* Will check if the packet is an ACK and if it has a correct sequence no
 * if not - the packet will be buffered */
radio_802154_rf_buffer_t *radio_802154_handle_ack(const struct radio_802154_driver *radio, const uint8_t *buf, int len);

/* The transmit will transmit with built-in retransmission and ACK handling */
int radio_802154_transmit(const struct radio_802154_driver *radio, unsigned short total_len);

void radio_802154_handle_collisions(int on);

#endif /* RADIO_802154_DEV_H_ */
