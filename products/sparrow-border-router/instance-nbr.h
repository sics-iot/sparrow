/*
 * Copyright (c) 2017, RISE SICS AB.
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
 */

/**
 * \file
 *         Neighbours instance
 * \author
 *         Bengt Ahlgren <bengta@sics.se>
 */

/*
 * Modelled after instance-rtable* by
 * Fredrik Ljungberg <flag@yanzinetworks.com>
 * Joakim Eriksson <joakime@sics.se>
 * Niclas Finne <nfi@sics.se>
 */

#ifndef _INSTANCE_NBR_H_
#define _INSTANCE_NBR_H_

#include <stdint.h>
#include "net/ip/uip.h"

/* NOTE: numbers kept in network byte order! */
struct nbr_entry {
  uip_ipaddr_t ipaddr;	     /* IP address */
#if NETSTACK_CONF_WITH_IPV6 == 0
  uint8_t      pad1[12];
#endif
  uint32_t     remaining;    /* Secs remaining of "reachable" timer */
  uint8_t      state;	     /* "REACHABLE" etc */
  uint8_t      rpl_flags;    /* RPL parent flags */
  uint16_t     rpl_rank;     /* RPL parent rank */
  uint16_t     link_etx;     /* ETX, fixed point w divisor 128 */
  int16_t      link_rssi;    /* RSSI */
  uint8_t      link_fresh;   /* Freshness */
  uint8_t      pad2[35];     /* Pad to even 64 bytes (room for extensions) */
};

typedef struct nbr_entry nbr_entry_t;

/* RPL parent flags (rpl_flags) */
#define NBR_FLAG_UPDATED           0x1 /* RPL_PARENT_FLAG_UPDATED (rpl.h) */
#define NBR_FLAG_LINK_METRIC_VALID 0x2 /* RPL_PARENT_FLAG_LINK_METRIC_VALID (rpl.h) */
#define NBR_FLAG_PARENT            0x4 /* Is RPL parent */
#define NBR_FLAG_PREFERRED         0x8 /* Is preferred RPL parent */

#endif /* _INSTANCE_NBR_H_ */
