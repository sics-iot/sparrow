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

#include "contiki-conf.h"
#include "sys/cc.h"
#include "net/ip/uip.h"
#include "net/netstack.h"
#include "net/mac/frame802154.h"
#include "net/mac/handler-802154.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/net-control.h"
#include "netscan.h"

#ifdef NETSCAN_CONF_LEDS
#define NETSCAN_LEDS NETSCAN_CONF_LEDS
#else
/* By default, use leds as activity indicator on platforms with leds */
#define NETSCAN_LEDS PLATFORM_HAS_LEDS
#endif

#if NETSCAN_LEDS
#include "dev/leds.h"

#if defined(LEDS_SYSTEM_GREEN) && LEDS_SYSTEM_GREEN
#define ACTIVITY_LED LEDS_SYSTEM_GREEN
#else
#define ACTIVITY_LED LEDS_GREEN
#endif

#define LEDS_ON() leds_on(ACTIVITY_LED)
#define LEDS_OFF() leds_off(ACTIVITY_LED)
#define LEDS_TOGGLE() leds_toggle(ACTIVITY_LED)

#else /* NETSCAN_LEDS */

#define LEDS_ON()
#define LEDS_OFF()
#define LEDS_TOGGLE()

#endif /* NETSCAN_LEDS */

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

extern const struct netscan_selector NETSCAN_SELECTOR;

typedef enum {
  STATE_STOPPED,
  STATE_IDLE,
  STATE_SCANNING,
  STATE_WAITING_FOR_SELECTOR
} scan_state_t;

static void clear_all_network_state(void);
static int radio_scan_callback(int channel, frame802154_t *frame,
                               handler_802154_callback_action_t cba);

#if UIP_CONF_IPV6_RPL
static net_control_key_t control_key = NET_CONTROL_KEY_NONE;
#endif /* UIP_CONF_IPV6_RPL */
static struct ctimer netscan_timer;
static scan_state_t state = STATE_STOPPED;
/*----------------------------------------------------------------*/
static CC_INLINE void
suppress_dio_output(uint8_t suppress)
{
#if UIP_CONF_IPV6_RPL
  net_control_set(NET_CONTROL_RPL_SUPPRESS_DIO, control_key, suppress != 0);
#endif /* UIP_CONF_IPV6_RPL */
  if(suppress) {
    LEDS_ON();
  } else {
    LEDS_OFF();
  }
}
/*----------------------------------------------------------------*/
static void
timeout(void *ptr)
{
  if(state == STATE_IDLE) {
    /* If we have no network by now, we should start a scan */
    if(NETSCAN_SELECTOR.is_done == NULL || NETSCAN_SELECTOR.is_done()) {
      netscan_stop();
    } else {
      netscan_start();
    }
  }
}
/*----------------------------------------------------------------*/
/*
 * Start a new scan
 */
void
netscan_start(void)
{
  PRINTF("netscan: Starting RF scan\n");

  suppress_dio_output(TRUE);
  clear_all_network_state();

  state = STATE_SCANNING;
  handler_802154_active_scan(radio_scan_callback);
}
/*----------------------------------------------------------------*/
/*
 * Stop the current scan
 */
void
netscan_stop(void)
{
  PRINTF("netscan: stopped\n");

  suppress_dio_output(FALSE);
  state = STATE_STOPPED;
}
/* ----------------------------------------------------------------*/
void
netscan_select_network(int channel, uint16_t panid)
{
  clear_all_network_state();
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel);
  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, panid);

#if UIP_CONF_IPV6_RPL
  /* Join this net and send DODAG Information Solicitation */
  dis_output(NULL);
#endif /* UIP_CONF_IPV6_RPL */
}
/* ----------------------------------------------------------------*/
int
netscan_set_beacon_payload(const uint8_t *payload, uint8_t payload_length)
{
#if UIP_CONF_IPV6_RPL
  /* if this node is routing it will need to send beacons */
  if(!RPL_LEAF_ONLY) {
    handler_802154_set_beacon_payload(payload, payload_length);
    return 1;
  }
#endif /* UIP_CONF_IPV6_RPL */
  return 0;
}
/* ----------------------------------------------------------------*/
/*
 * Callback from scan, called for each beacon reply received.
 */
static int
radio_scan_callback(int channel, frame802154_t *frame,
                    handler_802154_callback_action_t cba)
{
  uint8_t final_channel;

  /* no handling of beacon unless we do a scan */
  if((cba == CALLBACK_ACTION_RX) && (state != STATE_SCANNING) &&
     (state != STATE_WAITING_FOR_SELECTOR)) {
    PRINTF("netscan: Received beacon when not scanning (%d)\n", state);
    return CALLBACK_STATUS_CONTINUE;
  }

  if(state == STATE_STOPPED || state == STATE_IDLE) {
    return CALLBACK_STATUS_FINISHED;
  }

  if(NETSCAN_SELECTOR.is_done && NETSCAN_SELECTOR.is_done()) {
    PRINTF("netscan: selector is done!\n");
    netscan_stop();
    return CALLBACK_STATUS_FINISHED;
  }

  if(DEBUG) {
    switch(cba) {
    case CALLBACK_ACTION_SCAN_END:
      PRINTF("===== scan callback: SCAN_END =====\n");
      break;
    case CALLBACK_ACTION_RX:
      PRINTF("===== scan callback: RX ch %d =====\n", channel);
      break;
    case CALLBACK_ACTION_CHANNEL_DONE:
      PRINTF("===== scan callback: CHANNEL_DONE ch %d ===== \n", channel);
      break;
    case CALLBACK_ACTION_CHANNEL_DONE_LAST_CHANNEL:
      PRINTF("===== scan callback: CHANNEL_DONE LAST_CHANNEL %d =====\n",
             channel);
      break;
    }
  }

  switch(cba) {
  case CALLBACK_ACTION_SCAN_END:
    PRINTF("netscan: Active scan completed.\n");

    if(state == STATE_SCANNING || state == STATE_WAITING_FOR_SELECTOR) {
      state = STATE_IDLE;
      ctimer_restart(&netscan_timer);
    }
    return CALLBACK_STATUS_CONTINUE;

  case CALLBACK_ACTION_RX:
    if((channel == 0) || (frame == NULL)) {
      PRINTF("netscan: Bad channel or frame in beacon rx callback.\n");
      return CALLBACK_STATUS_CONTINUE;
    }

    PRINTF("netscan: beacon ch %d pan id %04x from ", channel, frame->src_pid);
    PRINTLLADDR((uip_lladdr_t *)frame->src_addr);
    PRINTF("\n");

    if(NETSCAN_SELECTOR.beacon_received) {
      switch(NETSCAN_SELECTOR.beacon_received(channel, frame->src_pid,
                                              (linkaddr_t *)frame->src_addr,
                                              frame->payload,
                                              frame->payload_len)) {
      case NETSCAN_CONTINUE:
        return CALLBACK_STATUS_CONTINUE;
      case NETSCAN_WAIT:
        state = STATE_WAITING_FOR_SELECTOR;
        return CALLBACK_STATUS_NEED_MORE_TIME;
      case NETSCAN_STOP:
        PRINTF("netscan: SELECTOR picked channel %d, pan id 0x%04x\n",
               channel, frame->src_pid);
        netscan_stop();
        return CALLBACK_STATUS_FINISHED;
      }
    }
    return CALLBACK_STATUS_CONTINUE;

  case CALLBACK_ACTION_CHANNEL_DONE:
  case CALLBACK_ACTION_CHANNEL_DONE_LAST_CHANNEL:
    LEDS_TOGGLE();

    if(cba == CALLBACK_ACTION_CHANNEL_DONE_LAST_CHANNEL) {
      final_channel = TRUE;
    } else {
      final_channel = FALSE;
    }

    if(NETSCAN_SELECTOR.channel_done) {
      switch(NETSCAN_SELECTOR.channel_done(channel, final_channel)) {
      case NETSCAN_CONTINUE:
        /* Continue with the scan */
        break;
      case NETSCAN_WAIT:
        state = STATE_WAITING_FOR_SELECTOR;
        return CALLBACK_STATUS_NEED_MORE_TIME;
      case NETSCAN_STOP:
        PRINTF("netscan: SELECTOR is done\n");
        netscan_stop();
        return CALLBACK_STATUS_FINISHED;
      }
    }

    PRINTF("netscan: leaving channel %d\n", channel);
    /* This will cause channel change, clear routes */
    clear_all_network_state();

    if(final_channel == TRUE) {
      LEDS_OFF();
      state = STATE_IDLE;
      ctimer_restart(&netscan_timer);
    }

    return CALLBACK_STATUS_CONTINUE;
  } /* switch(cba) */

  return CALLBACK_STATUS_CONTINUE;
}
/*----------------------------------------------------------------*/
static int
remove_all_neighbors(void)
{
  uip_ds6_nbr_t *nbr;
  int n;

  n = 0;
  while((nbr = nbr_table_head(ds6_neighbors)) != NULL) {
    uip_ds6_nbr_rm(nbr);
    n++;
  }
  return n;
}
/*----------------------------------------------------------------*/
static void
clear_all_network_state(void)
{
  uip_ds6_route_t *r;
  int n;
#if UIP_CONF_IPV6_RPL
  rpl_dag_t *dag;

  n = 0;
  while((dag = rpl_get_any_dag())) {
    if(dag->instance) {
      rpl_free_instance(dag->instance);
    }
    n++;
  }
  if(n > 0) {
    PRINTF("netscan: clear network state: %d DAG%s freed\n", n, (n == 1) ? "" : "s");
  }
#endif /* UIP_CONF_IPV6_RPL */

  n = 0;
  while((r = uip_ds6_route_head())) {
    uip_ds6_route_rm(r);
    n++;
  }
  if(n > 0) {
    PRINTF("netscan: clear network state: %d ROUTE%s freed\n", n, (n == 1) ? "" : "s");
  }

  n = remove_all_neighbors();
  if(n > 0) {
    PRINTF("netscan: clear network state: removed %d neighbor%s\n", n, (n == 1) ? "" : "s");
  }
}
/*----------------------------------------------------------------*/
void
netscan_init(void)
{
#if UIP_CONF_IPV6_RPL
  if(control_key == NET_CONTROL_KEY_NONE) {
    control_key = net_control_alloc_key();
    if(control_key == NET_CONTROL_KEY_NONE) {
      PRINTF("netscan: failed to allocate RPL control key\n");
    } else {
      state = STATE_IDLE;
    }
  }
#endif /* UIP_CONF_IPV6_RPL */

  ctimer_set(&netscan_timer, CLOCK_SECOND * 15, timeout, NULL);
}
/*---------------------------------------------------------------------------*/
