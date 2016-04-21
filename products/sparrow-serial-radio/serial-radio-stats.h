/*
 * Copyright (c) 2014-2016, SICS, Swedish ICT AB
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
 *         Serial radio statistics
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef SERIAL_RADIO_STATS_H_
#define SERIAL_RADIO_STATS_H_

#include "contiki-conf.h"

enum {
  SERIAL_RADIO_STATS_ENCAP_RECV,
  SERIAL_RADIO_STATS_ENCAP_TLV,
  SERIAL_RADIO_STATS_ENCAP_SERIAL,
  SERIAL_RADIO_STATS_ENCAP_UNPROCESSED,
  SERIAL_RADIO_STATS_ENCAP_ERRORS,

  SERIAL_RADIO_STATS_MAX
};

extern uint32_t serial_radio_stats[];

#define SERIAL_RADIO_STATS_INC(x) do {                                  \
    uint32_t tmp_radio_stats = ntohl(serial_radio_stats[x]);            \
    tmp_radio_stats++;                                                  \
    serial_radio_stats[x] = htonl(tmp_radio_stats);                     \
  } while(0)
#define SERIAL_RADIO_STATS_ADD(x, y) do {                               \
    uint32_t tmp_radio_stats = ntohl(serial_radio_stats[x]);            \
    tmp_radio_stats += y;                                               \
    serial_radio_stats[x] = htonl(tmp_radio_stats);                     \
  } while(0)
#define SERIAL_RADIO_STATS_GET(x) (ntohl(serial_radio_stats[x]))

enum {
  SERIAL_RADIO_STATS_DEBUG_SLIP_RECV,
  SERIAL_RADIO_STATS_DEBUG_SLIP_FRAMES,
  SERIAL_RADIO_STATS_DEBUG_SLIP_DROPPED,
  SERIAL_RADIO_STATS_DEBUG_SLIP_OVERFLOWS,
  SERIAL_RADIO_STATS_DEBUG_SLIP_ERRORS,

  SERIAL_RADIO_STATS_DEBUG_MAX
};

extern uint32_t serial_radio_stats_debug[];

#define SERIAL_RADIO_STATS_DEBUG_INC(x) do {                            \
    uint32_t tmp_radio_stats = ntohl(serial_radio_stats_debug[x]);      \
    tmp_radio_stats++;                                                  \
    serial_radio_stats_debug[x] = htonl(tmp_radio_stats);               \
  } while(0)
#define SERIAL_RADIO_STATS_DEBUG_ADD(x, y) do {                         \
    uint32_t tmp_radio_stats = ntohl(serial_radio_stats_debug[x]);      \
    tmp_radio_stats += y;                                               \
    serial_radio_stats_debug[x] = htonl(tmp_radio_stats);               \
  } while(0)
#define SERIAL_RADIO_STATS_DEBUG_GET(x) (ntohl(serial_radio_stats_debug[x]))

#endif /* SERIAL_RADIO_STATS_H_ */
