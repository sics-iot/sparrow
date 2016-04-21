/*
 * Copyright (c) 2013-2016, Yanzi Networks AB.
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
 *
 */

/**
 * \file
 *         A MAC handler for IEEE 802.15.4
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "net/mac/framer-802154.h"
#include "net/mac/frame802154.h"
#include "net/mac/handler-802154.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/net-control.h"
#include <string.h>
#include <stdio.h>

#ifdef HANDLER_802154_CONF_ACTIVE_SCAN_TIME
#define HANDLER_802154_ACTIVE_SCAN_TIME HANDLER_802154_CONF_ACTIVE_SCAN_TIME
#else /* HANDLER_802154_CONF_ACTIVE_SCAN_TIME */
#define HANDLER_802154_ACTIVE_SCAN_TIME ((CLOCK_SECOND * 6) / 10)
#endif /* HANDLER_802154_CONF_ACTIVE_SCAN_TIME */

#ifndef HANDLER_802154_DISABLE_RANDOM_CHANNEL
#define HANDLER_802154_DISABLE_RANDOM_CHANNEL 0
#endif /* HANDLER_802154_DISABLE_RANDOM_CHANNEL */

#define CHANNEL_LOW  11
#define CHANNEL_HIGH 26

#if HANDLER_802154_CONF_STATS
handler_802154_stats_t handler_802154_stats;
#endif /* HANDLER_802154_CONF_STATS */

/* ----------------------------------------------------- */
/* variables for beacon req/resp - active scan */
/* ----------------------------------------------------- */
static int current_channel = 0;
static int last_channel = 0;
static uint8_t chseqno = 0;
static int scan = 0; /* flag if scan is already active */
static struct ctimer scan_timer;
static struct ctimer beacon_send_timer;

static void handle_beacon(frame802154_t *frame);
static void handle_beacon_send_timer(void *p);
static scan_callback_t callback;
static uint8_t beacon_payload[HANDLER_802154_BEACON_PAYLOAD_BUFFER_SIZE] = { 0xfe, 0x00 };
static uint8_t beacon_payload_len = 0;
static int others_beacon_reply;

static net_control_key_t net_key;

#if (HANDLER_802154_DISABLE_RANDOM_CHANNEL == 0)
uint8_t channel_list[CHANNEL_HIGH - CHANNEL_LOW + 1];
uint8_t channel_index;
#endif /* (HANDLER_802154_DISABLE_RANDOM_CHANNEL == 0) */
/*---------------------------------------------------------------------------*/
/* the init function assumes that the framer 802.15.4 is active and
   used */
void
handler_802154_join(uint16_t pid)
{
  frame802154_set_pan_id(pid);
}
/*---------------------------------------------------------------------------*/
static void
end_scan(void)
{
  if(scan == 0) {
    /* Not scanning */
    return;
  }

  scan = 0;
  NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE,
                           RADIO_RX_MODE_ADDRESS_FILTER | RADIO_RX_MODE_AUTOACK);
  /* allow beacons again */
  net_control_set(NET_CONTROL_SUPPRESS_BEACON, net_key, 0);
  if(callback) {
    /* nothing more will be scanned now */
    callback(0, NULL, CALLBACK_ACTION_SCAN_END);
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_beacon(frame802154_t *frame)
{
  radio_value_t value;

  if(!net_control_get(NET_CONTROL_SUPPRESS_BEACON) &&
     (frame->payload_len == beacon_payload_len) &&
     (memcmp(beacon_payload, frame->payload, beacon_payload_len) == 0)) {
    others_beacon_reply++;
    if(others_beacon_reply >= 3) {
      /* Abort scheduled beacon transmit */
      ctimer_stop(&beacon_send_timer);
    }
  }

  /* scanned channel is already increased. */
  if(callback) {
    /* pick the channel from the RADIO if it have been changed since we started
       the scan - e.g. if we get a beacon later */
    NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &value);
    if(callback(value, frame, CALLBACK_ACTION_RX) == CALLBACK_STATUS_FINISHED) {
      /* scan is over - application is satisfied */
      end_scan();
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_beacon_send_timer(void *p)
{
  frame802154_t params;
  uint8_t len;

  /* Can we send a beacon or not - check payload len and suppression */
  if(beacon_payload_len == 0 || net_control_get(NET_CONTROL_SUPPRESS_BEACON)) {
    return;
  }

  /* init to zeros */
  memset(&params, 0, sizeof(params));

  /* use packetbuf for sending ?? */
  packetbuf_clear();
  /* Build the FCF. */
  params.fcf.frame_type = FRAME802154_BEACONFRAME;

  /* Insert IEEE 802.15.4 (2006) version bits. */
  params.fcf.frame_version = FRAME802154_IEEE802154_2006;

  /* assume long for now */
  params.fcf.src_addr_mode = FRAME802154_LONGADDRMODE;
  linkaddr_copy((linkaddr_t *)&params.src_addr, &linkaddr_node_addr);

  /* Set the source PAN ID to the global variable. */
  params.src_pid = frame802154_get_pan_id();

  params.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
  params.dest_addr[0] = 0xFF;
  params.dest_addr[1] = 0xFF;

  params.dest_pid = 0xffff;

  params.seq = frame802154_next_seqno();

  packetbuf_copyfrom(beacon_payload, beacon_payload_len);

  /* Set payload and payload length */
  params.payload = packetbuf_dataptr();
  params.payload_len = packetbuf_datalen();

  len = frame802154_hdrlen(&params);
  if(packetbuf_hdralloc(len)) {
    frame802154_create(&params, packetbuf_hdrptr(), len);
    NETSTACK_RADIO.send(packetbuf_hdrptr(), packetbuf_totlen());

    HANDLER_802154_STAT(handler_802154_stats.beacons_sent++);
  }
}
/*---------------------------------------------------------------------------*/
/* called to send a beacon request */
void
handler_802154_send_beacon_request(void)
{
  frame802154_t params;
  uint8_t len;

  /* init to zeros */
  memset(&params, 0, sizeof(params));

  /* use packetbuf for sending ?? */
  packetbuf_clear();
  /* Build the FCF. */
  params.fcf.frame_type = FRAME802154_CMDFRAME;

  /* Insert IEEE 802.15.4 (2006) version bits. */
  params.fcf.frame_version = FRAME802154_IEEE802154_2006;

  params.fcf.src_addr_mode = FRAME802154_NOADDR;

  params.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
  params.dest_addr[0] = 0xFF;
  params.dest_addr[1] = 0xFF;

  params.dest_pid = 0xffff;

  params.seq = chseqno;

  packetbuf_set_datalen(1);
  params.payload = packetbuf_dataptr();
  /* set the type in the payload */
  params.payload[0] = FRAME802154_BEACONREQ;
  params.payload_len = packetbuf_datalen();
  len = frame802154_hdrlen(&params);

  if(packetbuf_hdralloc(len)) {
    frame802154_create(&params, packetbuf_hdrptr(), len);
    NETSTACK_RADIO.send(packetbuf_hdrptr(), packetbuf_totlen());

    HANDLER_802154_STAT(handler_802154_stats.beacons_reqs_sent++);
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_scan_timer(void *p)
{
  if(!scan) {
    return;
  }

  if(scan > 1 && callback != NULL) {
    int callback_status;
    handler_802154_callback_action_t cba = CALLBACK_ACTION_CHANNEL_DONE;
    if(current_channel == last_channel) {
      cba = CALLBACK_ACTION_CHANNEL_DONE_LAST_CHANNEL;
    }
    callback_status = callback(current_channel, NULL, cba);
    if(callback_status == CALLBACK_STATUS_FINISHED) {
      end_scan();
      return;
    } else if(callback_status == CALLBACK_STATUS_NEED_MORE_TIME) {
      ctimer_reset(&scan_timer);
      return;
    }
  }

  scan++;

#if HANDLER_802154_DISABLE_RANDOM_CHANNEL
  if(current_channel == CHANNEL_HIGH) {
    /* Last channel has been scanned. Report that the process is over. */
    end_scan();
    return;
  }

  if(current_channel == 0) {
    current_channel = CHANNEL_LOW;
    last_channel == CHANNEL_HIGH;
  } else {
    current_channel++;
  }
#else /* HANDLER_802154_DISABLE_RANDOM_CHANNEL */
  if(channel_index > (CHANNEL_HIGH - CHANNEL_LOW)) {
    /* Last channel has been scanned. Report that the process is over. */
    end_scan();
    return;
  }

  if(current_channel == 0) {
    /* new scan, randomize channels */
    int i;
    int j;
    channel_index = 0;

    /* Initialize a random list as shown by Durstenfelds' Fisher–Yates shuffle. */
    for(i = 0; i <= (CHANNEL_HIGH - CHANNEL_LOW); i++) {
      j = random_rand() % (i + 1);
      if(j != i) {
        channel_list[i] = channel_list[j];
      }
      channel_list[j] = i + CHANNEL_LOW;
    }
    last_channel = channel_list[(CHANNEL_HIGH - CHANNEL_LOW)];
#if 0
    printf("New netscan search: channel order: ");
    for(i = 0; i <= (CHANNEL_HIGH - CHANNEL_LOW); i++) {
      printf("%d ", channel_list[i]);
    }
    printf("\n");
#endif /* 0 */
  }
  current_channel = channel_list[channel_index++];
#endif /* HANDLER_802154_DISABLE_RANDOM_CHANNEL */

  NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, 0);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, current_channel);

  handler_802154_send_beacon_request();

  ctimer_reset(&scan_timer);
}
/*---------------------------------------------------------------------------*/
/*
 * active scan - will scan all 16 channels in the 802.15.4 network
 * NOTE: this assumes 16 channels, starting from 11. Needs to be changed
 * if running on other than 2.4 GHz
 */
int
handler_802154_active_scan(scan_callback_t cb)
{
  if(net_key == NET_CONTROL_KEY_NONE) {
    net_key = net_control_alloc_key();
  }

  /* if no scan is active - start one */
  if(!scan) {
    callback = cb;
    current_channel = 0;
#if (HANDLER_802154_DISABLE_RANDOM_CHANNEL == 0)
    channel_index = 0;
#endif /* (HANDLER_802154_DISABLE_RANDOM_CHANNEL == 0) */
    scan = 1;
    /* suppress beacons during scan */
    net_control_set(NET_CONTROL_SUPPRESS_BEACON, net_key, 1);
    chseqno = frame802154_next_seqno();
    NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, 0);
    ctimer_set(&scan_timer, HANDLER_802154_ACTIVE_SCAN_TIME,
               &handle_scan_timer, callback);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
handler_802154_frame_received(frame802154_t *frame)
{
  if(beacon_payload_len > 0 && !net_control_get(NET_CONTROL_SUPPRESS_BEACON) &&
     frame->fcf.frame_type == FRAME802154_CMDFRAME &&
     frame->payload[0] == FRAME802154_BEACONREQ) {
    others_beacon_reply = 0;
    ctimer_set(&beacon_send_timer,
               CLOCK_SECOND / 16 +
               (CLOCK_SECOND * (random_rand() & 0xff)) / 0x200,
               &handle_beacon_send_timer, NULL);
    return 1;
  }
  if(frame->fcf.frame_type == FRAME802154_BEACONFRAME) {
    HANDLER_802154_STAT(handler_802154_stats.beacons_received++);
    handle_beacon(frame);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
handler_802154_set_beacon_payload(const uint8_t *payload, uint8_t len)
{
  if(len <= HANDLER_802154_BEACON_PAYLOAD_BUFFER_SIZE) {
    beacon_payload_len = 1;
    memset(beacon_payload, 0, HANDLER_802154_BEACON_PAYLOAD_BUFFER_SIZE);
    memcpy(beacon_payload, payload, len);
    beacon_payload_len = len;
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/*
 * Write current beacon payload length to "len", if non-NULL.
 *
 * Return pointer to beacon payload buffer.
 */
uint8_t *
handler_802154_get_beacon_payload(uint8_t *len)
{
  if(len) {
    *len = beacon_payload_len;
  }
  return beacon_payload;
}
/*---------------------------------------------------------------------------*/
