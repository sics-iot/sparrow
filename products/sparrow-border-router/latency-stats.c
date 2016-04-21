/*
 * Copyright (c) 2015-2016, Yanzi Networks AB.
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
 *         Latency statistics
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "latency-stats.h"
#include <string.h>
#include "net/ip/uip.h"

#define YLOG_LEVEL YLOG_LEVEL_DEBUG
#define YLOG_NAME  "latency"
#include "ylog.h"

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

/* We assume that we only record the MAC part of the address ... */
struct stat_entry {
  uip_ipaddr_t src;
  uip_ipaddr_t dest;
  clock_time_t time;
  uint8_t flag;
};

#define MAX_ENTRIES 16
static struct stat_entry entries[MAX_ENTRIES];
static uint8_t output_level = 0;
/*---------------------------------------------------------------------------*/
uint8_t
latency_stats_get_output_level(void)
{
  return output_level;
}
/*---------------------------------------------------------------------------*/
void
latency_stats_set_output_level(uint8_t level)
{
  output_level = level;
}
/*---------------------------------------------------------------------------*/
/* print latency statistics for pairs of packets (in/out) */
void
latency_stats_handle_packet(const uint8_t *buf, int size, uint8_t topan)
{
  clock_time_t now;
  const struct uip_ip_hdr *ipbuf = (const struct uip_ip_hdr *)buf;
  int found = 0;
  int i;

  if(output_level == 0) {
    /* Latency stats is currently disabled */
    return;
  }

  /* No need to check for non IP packets */
  if(size < 40) {
    return;
  }

  now = clock_time();

  /* First check if there is a fresh entry for this... */
  for(i = 0; i < MAX_ENTRIES; i++) {
    if(entries[i].flag > 0) {
      /* Just keep track of things that take maximum 2 seconds */
      if(now - entries[i].time < CLOCK_SECOND * 2) {
        if(!found &&
           uip_ipaddr_cmp(&ipbuf->destipaddr, &entries[i].src) &&
           uip_ipaddr_cmp(&ipbuf->srcipaddr, &entries[i].dest)) {
          /* This is a likely response on the other packet!!! */
          YLOG_INFO("Response time: %4u ", (unsigned int)(now - entries[i].time));
          PRINT6ADDR(&entries[i].src);
          PRINTF(" <-> ");
          PRINT6ADDR(&entries[i].dest);
          PRINTA("\n");
          found = 1;
          entries[i].flag = 0;
        }
      } else {
        /* more than two seconds => timeout */
        entries[i].flag = 0;
      }
    }
  }

  /* nothing found - this might be a new req - response thing? */
  if(found == 0) {
    for(i = 0; i < MAX_ENTRIES; i++) {
      if(entries[i].flag == 0) {
        entries[i].flag = LATENCY_STATISTICS_USED | topan;
        uip_ipaddr_copy(&entries[i].src, &ipbuf->srcipaddr);
        uip_ipaddr_copy(&entries[i].dest, &ipbuf->destipaddr);
        entries[i].time = now;
        return;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
