/*
 * Copyright (c) 2014-2016, Yanzi Networks AB.
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
 *         Instance for network statistics
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/mac/handler-802154.h"
#include "net/nbr-table.h"
#include "net/net-control.h"
#include "sparrow-oam.h"
#include "sparrow-uc.h"
#include "instance-nstats.h"
#include <stdlib.h>
#ifdef HAVE_NETSELECT
#include "netselect.h"
#endif /* HAVE_NETSELECT */
#include "instance-nstats-var.h"

SPARROW_OAM_INSTANCE_NAME(instance_nstats);

#ifndef INSTANCE_NSTATS_WITH_PUSH
#define INSTANCE_NSTATS_WITH_PUSH 1
#endif /* INSTANCE_NSTATS_WITH_PUSH */

#ifndef INSTANCE_NSTATS_PERIODIC
#define INSTANCE_NSTATS_PERIODIC 0
#endif

#define NSTATS_MIN_PUSH_PERIOD           30
#define NSTATS_MAX_DEFAULT_PAYLOAD      256

#define NS_LOLLIPOP_MAX_VALUE           255
#define NS_LOLLIPOP_CIRCULAR_REGION     127
#define NS_LOLLIPOP_SEQUENCE_WINDOWS     16
#define NS_LOLLIPOP_INIT                (NS_LOLLIPOP_MAX_VALUE - NS_LOLLIPOP_SEQUENCE_WINDOWS + 1)
#define NS_LOLLIPOP_INCREMENT(counter)                           \
  do {                                                           \
    if((counter) > NS_LOLLIPOP_CIRCULAR_REGION) {                \
      (counter) = ((counter) + 1) & NS_LOLLIPOP_MAX_VALUE;       \
    } else {                                                     \
      (counter) = ((counter) + 1) & NS_LOLLIPOP_CIRCULAR_REGION; \
    }                                                            \
  } while(0)

#define MAX_CANDIDATE_SIZE 4
static uint64_t push_until = 0L;
static uint32_t push_period = INSTANCE_NSTATS_PERIODIC;
static uint16_t push_port = 0;
static uint8_t next_data_blob[16];
static uint8_t next_data_blob_count = 0;
static uint8_t push_seqno = NS_LOLLIPOP_INIT;

#if INSTANCE_NSTATS_WITH_PUSH
#define SEND_STATS_DEFAULT         ((void *)0)
static struct ctimer push_timer;
static void send_stats(void *ptr);
#endif /* INSTANCE_NSTATS_WITH_PUSH */
/*---------------------------------------------------------------------------*/
static size_t
init_blob(uint8_t *buf, size_t size, uint8_t data_count)
{
  if(size > 1) {
    buf[0] = INSTANCE_NSTATS_DATA_VERSION;
    buf[1] = data_count;
    memset(buf + 2, 0, size - 2);
    return 2;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static size_t
finish_blob(uint8_t *buf, size_t size, uint8_t data_count)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
/*
 * Write an integer value as 8 bit value to byte array.
 * The value is capped to be between 0 and 255.
 */
static void
write_int8_max_to_buf(uint8_t *p, int value)
{
  if(value > 255) {
    *p = 255;
  } else if(value > 0) {
    *p = value & 0xff;
  } else {
    *p = 0;
  }
}
/*---------------------------------------------------------------------------*/
static size_t
write_stats_rpl(uint8_t *reply, size_t len)
{
  network_stats_rpl_t *stats;
  rpl_instance_t *instance;
  rpl_parent_t *preferred_parent;
  uip_ipaddr_t *addr;
  uip_ds6_nbr_t *nbr;

  if(len < sizeof(network_stats_rpl_t)) {
    return 0;
  }
  stats = (network_stats_rpl_t *)reply;
  memset(stats, 0, sizeof(network_stats_rpl_t));

  stats->seqno = push_seqno;
  write_int8_max_to_buf(&stats->free_neighbors, NBR_TABLE_MAX_NEIGHBORS - uip_ds6_nbr_num());
  write_int8_max_to_buf(&stats->free_routes, UIP_DS6_ROUTE_NB - uip_ds6_route_num_routes());

#if RPL_CONF_STATS
  stats->parent_switches = rpl_stats.parent_switch & 0xff;
#endif

  instance = rpl_get_instance(RPL_DEFAULT_INSTANCE);
  if(instance != NULL) {
    if(instance->current_dag != NULL) {
      preferred_parent = instance->current_dag->preferred_parent;
      if(preferred_parent != NULL) {
        addr = rpl_get_parent_ipaddr(preferred_parent);
        nbr = rpl_get_nbr(preferred_parent);
        if(addr != NULL) {
          memcpy(stats->parent, &addr->u8[12], 4);
          sparrow_tlv_write_int16_to_buf(stats->parent_rank, preferred_parent->rank);
          if(nbr != NULL) {
            sparrow_tlv_write_int16_to_buf(stats->parent_metric, nbr->link_metric);
          }
        }
      }
      sparrow_tlv_write_int16_to_buf(stats->dag_rank, instance->current_dag->rank);
    }
    stats->dio_interval = instance->dio_intcurrent;
#if RPL_CONF_STATS
    stats->dio_sent = instance->dio_totsend & 0xff;
    /* stats->dao_received = instance->dao_totrecv & 0xff; */
    /* stats->dao_accepted = instance->dao_totaccepted & 0xff; */
    /* stats->dao_sent = instance->dao_totsent & 0xff; */
    /* stats->dao_nopath_sent = instance->dao_nopath_totsent & 0xff; */
    /* stats->dao_nack_sent = instance->dao_nack_totsent & 0xff; */
    /* stats->dao_loops_detected = instance->dao_totloops_detected & 0xff; */
    /* stats->ext_hdr_errors = (instance->fwd_toterrors + instance->rank_toterrors) & 0xff; */
#endif /* RPL_CONF_STATS */
  }

  write_int8_max_to_buf(&stats->oam_last_activity, sparrow_oam_get_time_since_last_good_rx_from_uc() / 60000UL);

  return sizeof(network_stats_rpl_t);
}
/*---------------------------------------------------------------------------*/
static size_t
write_stat(uint8_t type, uint8_t *reply, size_t len)
{
  switch(type) {
  case INSTANCE_NSTATS_DATA_RPL:
    return write_stats_rpl(reply, len);
  case INSTANCE_NSTATS_DATA_PARENT_INFO:
    /* TODO information about candidate parents */
    return 0;
  case INSTANCE_NSTATS_DATA_BEACONS:
#if HANDLER_802154_CONF_STATS
    if(len >= sizeof(network_stats_beacons_t)) {
      network_stats_beacons_t *b;
      b = (network_stats_beacons_t *)reply;
      sparrow_tlv_write_int16_to_buf(b->beacons_reqs_sent, handler_802154_stats.beacons_reqs_sent);
      sparrow_tlv_write_int16_to_buf(b->beacons_sent, handler_802154_stats.beacons_sent);
      sparrow_tlv_write_int16_to_buf(b->beacons_received, handler_802154_stats.beacons_received);
      return sizeof(network_stats_beacons_t);
    }
#endif /* HANDLER_802154_CONF_STATS */
    return 0;
  case INSTANCE_NSTATS_DATA_NETSELECT:
#if defined HAVE_NETSELECT && NETSELECT_CONF_STATS
    if(len >= sizeof(network_stats_netselect_t)) {
      network_stats_netselect_t *ns;
      ns = (network_stats_netselect_t *)reply;
      sparrow_tlv_write_int32_to_buf(ns->netselect_elapsed, netselect_stats.elapsed);
      sparrow_tlv_write_int32_to_buf(ns->netselect_last, (uint32_t)(netselect_time / 1000UL));
      ns->netselect_count = netselect_stats.count & 0xff;
      return sizeof(network_stats_netselect_t);
    }
#endif /* defined HAVE_NETSELECT && NETSELECT_CONF_STATS */
    return 0;
  case INSTANCE_NSTATS_DATA_RADIO:
    return 0;
  case INSTANCE_NSTATS_DATA_CONFIG:
    if(len >= sizeof(network_stats_config_t)) {
      network_stats_config_t *c;
      c = (network_stats_config_t *)reply;
      sparrow_tlv_write_int16_to_buf(c->routes, UIP_DS6_ROUTE_NB);
      sparrow_tlv_write_int16_to_buf(c->neighbors, NBR_TABLE_MAX_NEIGHBORS);
      sparrow_tlv_write_int16_to_buf(c->defroutes, UIP_DS6_DEFRT_NB);
      sparrow_tlv_write_int16_to_buf(c->prefixes, UIP_DS6_PREFIX_NB);
      sparrow_tlv_write_int16_to_buf(c->unicast_addresses, UIP_DS6_ADDR_NB);
      sparrow_tlv_write_int16_to_buf(c->multicast_addresses, UIP_DS6_MADDR_NB);
      sparrow_tlv_write_int16_to_buf(c->anycast_addresses, UIP_DS6_AADDR_NB);
      return sizeof(network_stats_config_t);
    }
    return 0;
  default:
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static size_t
write_stats_data_and_element_count(sparrow_tlv_t *request,
                                   uint8_t *reply, size_t len)
{
  uint8_t buf[256];
  size_t s, pos, size, n;
  int i;

  size = sizeof(buf);
  if(size > len) {
    size = len;
  }

  if(next_data_blob_count == 0) {
    /* Restore default */
    next_data_blob[0] = INSTANCE_NSTATS_DATA_RPL;
    next_data_blob_count = 1;
  }

  pos = init_blob(buf, size, next_data_blob_count);
  if(pos + next_data_blob_count * 2 > size) {
    /* Too little space */
    return sparrow_tlv_write_reply_error(request,
                                         SPARROW_TLV_ERROR_HARDWARE_ERROR,
                                         reply, len);
  }

  /* Add the included blob identifiers */
  s = pos;
  for(i = 0; i < next_data_blob_count; i++) {
    buf[pos++] = next_data_blob[i];
  }

  /* Write the blob data */
  for(i = 0; i < next_data_blob_count; i++) {
    n = write_stat(next_data_blob[i], buf + pos, size - pos);
    if(n == 0) {
      /* Something went wrong - ignore this data blob */
      buf[s + i] = INSTANCE_NSTATS_DATA_NONE;
    } else {
      pos += n;
    }
  }

  pos += finish_blob(buf + pos, size - pos, next_data_blob_count);

  /* Reset the next reply to default */
  next_data_blob_count = 0;

  request->element_size = 4;
  if(pos > 0) {
    request->elements = pos / request->element_size;
    if(pos & 3) {
      request->elements++;
    }
  } else {
    request->elements = 0;
  }
  return sparrow_tlv_write_reply_vector(request, reply, len, buf);
}
/*---------------------------------------------------------------------------*/
/**
 * Process a request TLV.
 *
 * Writes a response TLV starting at "reply", no more than "len" bytes.
 *
 * If the "request" is of type vectorAbortIfEqualRequest or
 * abortIfEqualRequest, and processing should be aborted,
 * "oam_processing" is set to SPARROW_OAM_PROCESSING_ABORT_TLV_STACK.
 *
 * Returns the number of bytes written to "reply".
 */
static size_t
process_request(const sparrow_oam_instance_t *instance,
                sparrow_tlv_t *request, uint8_t *reply, size_t len,
                sparrow_oam_processing_t *oam_processing)
{
  uint32_t local32;

  if(request->opcode == SPARROW_TLV_OPCODE_SET_REQUEST ||
     request->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST) {

#if INSTANCE_NSTATS_WITH_PUSH

    if(request->variable == VARIABLE_NSTATS_PUSH_PERIOD) {
      push_period = sparrow_tlv_get_int32_from_data(request->data);
      if(push_period == 0) {
        ctimer_stop(&push_timer);
      } else {
        /* Do not push too often */
        local32 = push_period;
        if(local32 < NSTATS_MIN_PUSH_PERIOD) {
          local32 = NSTATS_MIN_PUSH_PERIOD;
        }
        ctimer_set(&push_timer, local32 * CLOCK_SECOND, send_stats, SEND_STATS_DEFAULT);
      }
      return sparrow_tlv_write_reply32(request, reply, len, NULL);
    }

    if(request->variable == VARIABLE_NSTATS_PUSH_TIME) {
      local32 = sparrow_tlv_get_int32_from_data(request->data);
      if(local32 == 0) {
        push_until = 0ULL;
      } else {
        push_until = uptime_read() + local32 * 1000UL;
      }
      return sparrow_tlv_write_reply32(request, reply, len, NULL);
    }

    if(request->variable == VARIABLE_NSTATS_PUSH_PORT) {
      local32 = sparrow_tlv_get_int32_from_data(request->data);
      push_port = local32 & 0xffff;
      return sparrow_tlv_write_reply32(request, reply, len, NULL);
    }

#endif /* INSTANCE_NSTATS_WITH_PUSH */

    if(request->variable == VARIABLE_NSTATS_DATA) {
      if(request->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST) {
        if(request->offset > 0) {
          return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_BAD_NUMBER_OF_ELEMENTS, reply, len);
        }
        /* Clear next data types */
        next_data_blob_count = 0;
        if(request->elements == 0) {
          return sparrow_tlv_write_reply_vector(request, reply, len, NULL);
        }
        while(next_data_blob_count < sizeof(next_data_blob) &&
              next_data_blob_count + 2 < request->elements * 4 &&
              next_data_blob_count < request->data[1]) {
          next_data_blob[next_data_blob_count] = request->data[next_data_blob_count + 2];
          next_data_blob_count++;
        }
        return sparrow_tlv_write_reply_vector(request, reply, len, NULL);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_NO_VECTOR_ACCESS, reply, len);
      }
    }

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED, reply, len);
  } else if((request->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST)) {

    if(request->variable == VARIABLE_NSTATS_VERSION) {
      local32 = INSTANCE_NSTATS_VERSION;
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }

    if(request->variable == VARIABLE_NSTATS_CAPABILITIES) {
      uint64_t local64;
      local64 = 0ULL;

#if INSTANCE_NSTATS_WITH_PUSH
      local64 |= INSTANCE_NSTATS_CAPABILITY_PUSH;
#endif /* INSTANCE_NSTATS_WITH_PUSH */

#if !RPL_LEAF_ONLY
      local64 |= INSTANCE_NSTATS_CAPABILITY_ROUTER;
      if(!net_control_get(NET_CONTROL_RPL_SUPPRESS_DIO)) {
        local64 |= INSTANCE_NSTATS_CAPABILITY_ROUTING;
      }
#endif /* !RPL_LEAF_ONLY */

      return sparrow_tlv_write_reply64int(request, reply, len, local64);
    }

    if(request->variable == VARIABLE_NSTATS_PUSH_PERIOD) {
      local32 = push_period;
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }

    if(request->variable == VARIABLE_NSTATS_PUSH_TIME) {
      if(push_until == 0ULL || uptime_read() >= push_until) {
        local32 = 0UL;
      } else {
        local32 = (uint32_t)((push_until - uptime_read()) / 1000);
      }
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }

    if(request->variable == VARIABLE_NSTATS_PUSH_PORT) {
      local32 = push_port;
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }

    if(request->variable == VARIABLE_NSTATS_DATA) {
      if(request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST) {
        if(request->offset > 0) {
          /* Partial requests not allowed */
          request->elements = 0;
        }
        if(request->elements == 0) {
          /* Nothing requested */
          return sparrow_tlv_write_reply_vector(request, reply, len, NULL);
        }
        return write_stats_data_and_element_count(request, reply, len);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_NO_VECTOR_ACCESS, reply, len);
      }
    }

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }

  return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}
/*---------------------------------------------------------------------------*/
#if INSTANCE_NSTATS_WITH_PUSH
static void
send_stats(void *type)
{
  uint8_t txbuf[512];
  sparrow_tlv_t T;
  size_t txlen;
  sparrow_encap_pdu_info_t pinfo;
  uint16_t port;
  const uint8_t *p;
  uip_ipaddr_t destination_address;
  sparrow_oam_processing_t oam_processing = SPARROW_OAM_PROCESSING_NEW;
  int has_data = FALSE;

  ctimer_restart(&push_timer);

  if(push_until > 0 && uptime_read() > push_until) {
    /* Do not push data any more but keep the push timer running for now as it is
       controlled by the push period and we do not want to affect the timing. */
    return;
  }

  p = sparrow_uc_get_address();
  port = (p[7] << 8) | p[6];
  if(port == 0) {
    /* No unit controller assigned */
    return;
  }

  /* Send network stats to the unit controller */
  memcpy(&destination_address.u8[0], &p[8], 16);

  sparrow_encap_init_pdu_info_for_event(&pinfo);
  txlen = sparrow_encap_write_header(txbuf, sizeof(txbuf) - 2, &pinfo);

  if(type == SEND_STATS_DEFAULT) {
    has_data = TRUE;

    /* Update statistics seqno each time the statistics is sent -
       can be used to detect lost statistics */
    NS_LOLLIPOP_INCREMENT(push_seqno);

    memset(&T, 0, sizeof(T));
    T.version = SPARROW_TLV_VERSION;
    T.variable = VARIABLE_NSTATS_DATA;
    T.instance = sparrow_oam_get_instance_id(&instance_nstats);
    T.element_size = 4;
    T.opcode = SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST;
    T.elements = NSTATS_MAX_DEFAULT_PAYLOAD / 4;

    /* Reset the reply to default */
    next_data_blob_count = 0;

    txlen += process_request(&instance_nstats, &T,
                             &txbuf[txlen], sizeof(txbuf) - txlen - 2,
                             &oam_processing);
  }

  if(has_data) {
    /* null tlv */
    txbuf[txlen++] = 0;
    txbuf[txlen++] = 0;

    txlen = sparrow_encap_finalize(txbuf, sizeof(txbuf) - 2, &pinfo, txlen);

    sparrow_oam_send_udp_packet(&destination_address, port, txbuf, txlen);
  }
}
#endif /* INSTANCE_NSTATS_WITH_PUSH */
/*---------------------------------------------------------------------------*/
/*
 * Init called from sparrow_oam_init.
 */
#if INSTANCE_NSTATS_WITH_PUSH
static void
init(const sparrow_oam_instance_t *instance)
{
  if(push_period > 0) {
    if(push_period < NSTATS_MIN_PUSH_PERIOD) {
      push_period = NSTATS_MIN_PUSH_PERIOD;
    }
    ctimer_set(&push_timer, push_period * CLOCK_SECOND, send_stats, SEND_STATS_DEFAULT);
  }
}
#endif /* INSTANCE_NSTATS_WITH_PUSH */
/*---------------------------------------------------------------------------*/
SPARROW_OAM_INSTANCE(instance_nstats,
                     INSTANCE_NSTATS_OBJECT_TYPE, INSTANCE_NSTATS_LABEL,
                     instance_nstats_variables,
#if INSTANCE_NSTATS_WITH_PUSH
                     .init = init,
#endif /* INSTANCE_NSTATS_WITH_PUSH */
                     .process_request = process_request);
/*---------------------------------------------------------------------------*/
