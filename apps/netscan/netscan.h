/*
 * Copyright (c) 2013-2016, Yanzi Networks AB.
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
 *         Network scan functions for IEEE 802.15.4
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef NETSCAN_H_
#define NETSCAN_H_

#include "contiki-conf.h"
#include "net/linkaddr.h"

#ifndef NETSCAN_SELECTOR
#ifdef NETSCAN_CONF_SELECTOR
#define NETSCAN_SELECTOR NETSCAN_CONF_SELECTOR
#else
#define NETSCAN_SELECTOR netscan_default_selector
#endif /* NETSCAN_CONF_SELECTOR */
#endif /* NETSCAN_SELECTOR */

typedef enum {
  NETSCAN_CONTINUE,
  NETSCAN_WAIT,
  NETSCAN_STOP
} netscan_action_t;

void netscan_init(void);
void netscan_start(void);
void netscan_stop(void);
void netscan_select_network(int channel, uint16_t panid);
int netscan_set_beacon_payload(const uint8_t *payload, uint8_t payload_length);

struct netscan_selector {
  void (* init)(void);
  int  (* is_done)(void);
  netscan_action_t (* beacon_received)(int channel, uint16_t panid,
                                       const linkaddr_t *source,
                                       const uint8_t *payload,
                                       uint8_t length);
  netscan_action_t (* channel_done)(int channel, int is_last_channel);
};

#endif /* NETSCAN_H_ */
