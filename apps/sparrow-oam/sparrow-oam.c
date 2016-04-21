/*
 * Copyright (c) 2011-2016, Yanzi Networks AB.
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
 *         OAM module for Sparrow application layer
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Oriol Piñol Piñol <oriol.pinol@yanzinetworks.com>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

/**
 * How to use the Sparrow OAM module:
 *
 * 1: In your applications project-conf.h add
 *
 * #define PRODUCT_TYPE_INT64 0x0090DA0301010470ULL
 * #define PRODUCT_LABEL "The product label that will be show in UI"
 *
 *
 * 2: When adding a new instance, generate a variable file using
 *    tools/sparrow/instance-gen.py (see wiki for more information)
 *
 *
 * 3: Implement an implement initialization function. This function is called
 *    at startup when Sparrow OAM is initiated and this function is optional.
 *
 *    static void anything_init(const sparrow_oam_instance_t *instance) {
 *       ...
 *    }
 *
 *
 * 4: Implement a process_request function. This function is called for all
 *    TLVs that match the instance, and should write a reply TLV to "reply".
 *
 *    static size_t
 *    process_request(const sparrow_oam_instance_t *instance,
 *                    sparrow_tlv_t *request, uint8_t *reply, size_t len,
 *                    sparrow_oam_processing_t *oam_processing) {
 *      ...
 *    }
 *
 *
 * 5: Implement a poll function. It is not necessary to write a reply
 *    to a poll and the function is optional.
 *
 *    static size_t
 *    poll(const sparrow_oam_instance_t *instance, uint8_t *reply, size_t len,
 *         sparrow_oam_poll_type_t poll_type) {
 *       ...
 *    }
 *
 *
 * 6: Setup the Sparrow OAM instance. All callback functions are optional
 *    but normally an instance should at least have the process_request().
 *
 *    SPARROW_OAM_INSTANCE(anything, object_type, object_label,
 *                         anything_variables, .init = anything_init,
 *                         .process_request = process_request,
 *                         .poll = poll);
 *
 * 7: Include the instance by adding it to the make variable
 *    SPARROW_OAM_INSTANCES. The order the instances are added decides
 *    the instance index used to address the instances.
 *
 *    Makefile:
 *    SPARROW_OAM_INSTANCES += anything
 */

#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ip/uip-udp-packet.h"
#include "net/ipv6/sicslowpan.h"
#include "sparrow-oam.h"
#include "sparrow-psp.h"
#include "sparrow-uc.h"
#include "sparrow-encap.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

#define TXBUF_SIZE 1280

#define UDP_HDR ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#ifdef VARIABLE_TLV_GROUPING
uint64_t tlv_grouping_time = 0;
uint32_t tlv_grouping_holdoff = 0;
#endif /* VARIABLE_TLV_GROUPING */

#ifndef SPARROW_OAM_INSTANCE0_NAME
#define SPARROW_OAM_INSTANCE0_NAME instance0
#endif /* SPARROW_OAM_INSTANCE0_NAME */

extern const sparrow_oam_instance_t SPARROW_OAM_INSTANCE0_NAME;

#ifdef SPARROW_OAM_DECLARATIONS
SPARROW_OAM_DECLARATIONS
#endif /* SPARROW_OAM_DECLARATIONS */

#ifdef SPARROW_OAM_LOCAL_DECLARATIONS
SPARROW_OAM_LOCAL_DECLARATIONS
#endif /* SPARROW_OAM_LOCAL_DECLARATIONS */

static const sparrow_oam_instance_t *instances[] = {
  &SPARROW_OAM_INSTANCE0_NAME,
#ifdef SPARROW_OAM_INSTANCES
  SPARROW_OAM_INSTANCES
#endif /* SPARROW_OAM_INSTANCES */
#ifdef SPARROW_OAM_LOCAL_INSTANCES
  SPARROW_OAM_LOCAL_INSTANCES
#endif /* SPARROW_OAM_LOCAL_INSTANCES */
  NULL
};
#define INSTANCE_COUNT (sizeof(instances) / sizeof(sparrow_oam_instance_t *) - 1)

static struct uip_udp_conn *oam_sock;
static void process_request(process_event_t ev, process_data_t data);
static void send_event(void);

static struct etimer oam_rx_timer;
static uint64_t last_oam_rx_from_uc = 0;
static uint64_t last_oam_rx = 0;
static uint64_t last_oam_tx = 0;
static uint8_t send_wakeup_now = FALSE;

PROCESS(oam, "OAM");
/*---------------------------------------------------------------------------*/
const sparrow_oam_instance_t *
sparrow_oam_get_instance(uint8_t instance_id)
{
  if(instance_id >= INSTANCE_COUNT) {
    return NULL;
  }
  return instances[instance_id];
}
/*---------------------------------------------------------------------------*/
const sparrow_oam_instance_t **
sparrow_oam_get_instances(void)
{
  return instances;
}
/*----------------------------------------------------------------*/
uint8_t
sparrow_oam_get_instance_count(void)
{
  return INSTANCE_COUNT;
}
/*----------------------------------------------------------------*/
uint8_t
sparrow_oam_get_instance_id(const sparrow_oam_instance_t *instance)
{
  int i;
  if(instance != NULL) {
    if(instance->data->instance_id < INSTANCE_COUNT &&
       instances[instance->data->instance_id] == instance) {
      return instance->data->instance_id;
    }

    for(i = 0; instances[i] != NULL; i++) {
      if(instances[i] == instance) {
        instance->data->instance_id = i;
        return i;
      }
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/*
 * Initialize all oam modules.
 */
void
sparrow_oam_init(void)
{
  static uint8_t init_done = FALSE;
  int i;

  if(init_done) {
    return;
  }

  for(i = 0; instances[i] != NULL; i++) {
    memset(instances[i]->data, 0, sizeof(sparrow_oam_instance_data_t));
    instances[i]->data->instance_id = i;
    if(instances[i]->init) {
      instances[i]->init(instances[i]);
    }
  }
  init_done = TRUE;

  process_start(&oam, NULL);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(oam, ev, data)
{
  static clock_time_t wait_time;
  uint64_t now;

  PROCESS_BEGIN();

  oam_sock = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(oam_sock, UIP_HTONS(SPARROW_OAM_PORT));

  while(TRUE) {
    now = uptime_read();

    /* Default to 3 seconds */
    wait_time = CLOCK_CONF_SECOND * 3;

    /* Then check for pending event */
    if(sparrow_oam_event_backoff_timer(SPARROW_OAM_EVENT_BACKOFF_TIMER_IS_ACTIVE)) {
      uint64_t next_event = sparrow_oam_event_backoff_timer(SPARROW_OAM_EVENT_BACKOFF_TIMER_GET_NEXT_TX_TIME);
      if(next_event < now) {
        wait_time = 1;
      } else {
        wait_time = next_event - now;
      }
    }

    /* now check for wake up event */
    if(send_wakeup_now) {
      wait_time = 1;
    }

    etimer_set(&oam_rx_timer, wait_time);

    PROCESS_WAIT_EVENT();

    if(ev == tcpip_event) {
      if(uip_newdata()) {
        process_request(ev, data);
      } else {
        printf("OAM  : tcpip event w/o new data\n");
      }
      continue;
    }

    /*
     * Here there is a poll or timeout. In both cases, check if it is
     * time to send event.
     */
    if(sparrow_oam_event_backoff_timer(SPARROW_OAM_EVENT_BACKOFF_TIMER_ARE_WE_THERE_YET) || send_wakeup_now) {
      send_event();
      /*
       * If a wakeup notification was schedued, it has been taken care
       * of by the event.
       */
      send_wakeup_now = FALSE;
      continue;
    }
  } /* while(TRUE) */
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*
 * TLV stacks
 */

/*
 * Stuff to do before starting processing of a new TLV stack.
 */
void
sparrow_oam_pdu_begin(void)
{
#ifdef VARIABLE_TLV_GROUPING
  if(uptime_elapsed(tlv_grouping_time) < tlv_grouping_holdoff) {
    return;
  }
#endif /* VARIABLE_TLV_GROUPING */

  /* Tell instances that has registered interest */
  sparrow_oam_notify_all_instances(SPARROW_OAM_NT_NEW_PDU, 0);
}
/*---------------------------------------------------------------------------*/

/*
 * Stuff to do after processing of a TLV stack.
 */
void
sparrow_oam_pdu_end(void)
{
  sparrow_oam_notify_all_instances(SPARROW_OAM_NT_END_PDU, 0);
}
/*---------------------------------------------------------------------------*/

/*
 * Stuff to do after sending reply.
 * sent_or_skipped should be non-zero if reply was sent.
 */
static void
reply_sent(uint8_t sent_or_skipped)
{
  /* Tell instances that has registered interest */
  sparrow_oam_notify_all_instances(SPARROW_OAM_NT_REPLY_SENT, sent_or_skipped);
}
/*---------------------------------------------------------------------------*/

static void
process_request(process_event_t ev, process_data_t data)
{
  size_t rxlen;
  uint8_t *txbuf;
  sparrow_encap_pdu_info_t pinfo;
  int32_t encap_header_length;
  uint8_t *buf;
  const sparrow_id_t *did;
  size_t l;
  size_t txsize;

  rxlen = uip_datalen();
  buf = uip_appdata;

  encap_header_length = sparrow_encap_parse_and_verify(buf, rxlen, &pinfo);
  if(encap_header_length < 0) {
    printf("OAM: decode message: error %d\n", (int)encap_header_length);
    return;
  }

  last_oam_rx = uptime_read();
  if(sparrow_uc_is_uc_address((uint8_t *)&UDP_HDR->srcipaddr, 16)) {
    last_oam_rx_from_uc = last_oam_rx;
  }
  PRINTF("oam: RX %d bytes\n", rxlen);

  txbuf = sicslowpan_alloc_fragbufs(TXBUF_SIZE, &txsize);
  if(txbuf == NULL) {
    /* Woops - no memory to allocate */
    printf("OAM: no tx buffer\n");
    return;
  }

  did = sparrow_get_did();
  pinfo.fpmode = SPARROW_ENCAP_FP_MODE_DEVID;
  pinfo.fp = &did->id[8];
  pinfo.fplen = 8;

  if(pinfo.payload_type == SPARROW_ENCAP_PAYLOAD_TLV) {
    /* process stack */
    l = sparrow_encap_write_header(txbuf, txsize - 2, &pinfo);
    l += sparrow_oam_process_request(buf + encap_header_length,
                                     rxlen - encap_header_length,
                                     txbuf + l, txsize - l - 2, &pinfo);

    /* add a null TLV */
    txbuf[l++] = 0;
    txbuf[l++] = 0;
  } else if(pinfo.payload_type == SPARROW_ENCAP_PAYLOAD_PORTAL_SELECTION_PROTO) {
    int local_len;
    l = sparrow_encap_write_header(txbuf, txsize - 2, &pinfo);
    local_len = sparrow_oam_process_request(buf + encap_header_length,
                                            rxlen - encap_header_length,
                                            txbuf + l, txsize - l - 2, &pinfo);
    if(local_len == 0) {
      l = 0;
    } else {
      l += local_len;
      /* add a null TLV */
      txbuf[l++] = 0;
      txbuf[l++] = 0;
    }
  } else if(pinfo.payload_type == SPARROW_ENCAP_PAYLOAD_ECHO_REQUEST) {
    pinfo.payload_type = SPARROW_ENCAP_PAYLOAD_ECHO_REPLY;
    l = sparrow_encap_write_header(txbuf, txsize - 4, &pinfo);
    memcpy(txbuf + l, buf + encap_header_length, 4);
    l += 4;
  } else if(pinfo.payload_type == SPARROW_ENCAP_PAYLOAD_ECHO_REPLY) {
    sparrow_oam_echo_replies(TRUE);
    l = 0; /* silent drop */
  } else {
    l = sparrow_encap_write_header_with_error(txbuf, txsize, &pinfo, SPARROW_ENCAP_ERROR_BAD_PAYLOAD_TYPE);
    l = 0;
  }

  l = sparrow_encap_finalize(txbuf, txsize, &pinfo, l);

  if(l > 0) {
    /* Send the reply datagram */
    uip_udp_packet_sendto(oam_sock, txbuf, l,
                          &UDP_HDR->srcipaddr, UDP_HDR->srcport);
    last_oam_tx = uptime_read();

    reply_sent(1);
    PRINTF("oam: TX %d bytes (reply)\n", l);
  } else {
    reply_sent(0);
  }
  /* free buffers */
  sicslowpan_free_fragbufs(txbuf);
}
/*---------------------------------------------------------------------------*/

uint8_t
sparrow_oam_echo_replies(uint8_t increment)
{
  static uint8_t replies = 0;
  if(increment) {
    replies++;
  }
  return replies;
}
/*---------------------------------------------------------------------------*/
void
sparrow_oam_send_udp_packet(const uip_ipaddr_t *destination, uint16_t port,
                            const uint8_t *data, int len)
{
  uip_udp_packet_sendto(oam_sock, data, len, destination, port);
}
/*---------------------------------------------------------------------------*/

static void
send_event(void)
{
  uint8_t *txbuf;
  size_t txlen;
  sparrow_encap_pdu_info_t pinfo;
  uint16_t port;
  const uint8_t *p;
  uip_ipaddr_t destination_address;
  size_t txsize;

  p = sparrow_uc_get_address();
  port = (p[7] << 8) | p[6];
  memcpy(&destination_address.u8[0], &p[8], 16);
  if(port == 0) {
    return;
  }
  txbuf = sicslowpan_alloc_fragbufs(TXBUF_SIZE, &txsize);
  if(txbuf == NULL) {
    /* Woops - no memory to allocate */
    return;
  }

  sparrow_encap_init_pdu_info_for_event(&pinfo);
  txlen = sparrow_encap_write_header(txbuf, txsize -2, &pinfo);
  txlen += sparrow_oam_generate_event_reply(&txbuf[txlen], txsize - txlen -2);
  /* null tlv */
  txbuf[txlen++] = 0;
  txbuf[txlen++] = 0;

  txlen = sparrow_encap_finalize(txbuf, txsize, &pinfo, txlen);

  if(txlen > 0) {
    uip_udp_packet_sendto(oam_sock, txbuf, txlen, &destination_address, port);
    last_oam_tx = uptime_read();
  }

  /* free the buffer */
  sicslowpan_free_fragbufs(txbuf);
}
/*---------------------------------------------------------------------------*/

void
sparrow_oam_schedule_wakeup_notification()
{
  if(!send_wakeup_now) {
    send_wakeup_now = TRUE;
    process_poll(&oam);
  }
}
/*---------------------------------------------------------------------------*/

/*
 * register transmit activity.
 */
void
sparrow_oam_tx_activity(void)
{
  last_oam_tx = uptime_read();
}
/*---------------------------------------------------------------------------*/

/*
 * Get time stamp of last oam activity.
 */
uint64_t
sparrow_oam_last_activity(sparrow_oam_activity_type_t type)
{
  uint64_t retval = 0;

  switch(type) {

  case SPARROW_OAM_ACTIVITY_TYPE_TX:
    retval = last_oam_tx;
    break;

  case SPARROW_OAM_ACTIVITY_TYPE_RX:
    retval = last_oam_rx;
    break;

  case SPARROW_OAM_ACTIVITY_TYPE_RX_FROM_UC:
    retval = last_oam_rx_from_uc;
    break;

  case SPARROW_OAM_ACTIVITY_TYPE_ANY:
    if(last_oam_tx > last_oam_rx) {
      retval = last_oam_tx;
    } else {
      retval = last_oam_rx;
    }
    break;
  }
  return retval;
}
/*---------------------------------------------------------------------------*/

/*
 * Get milliseconds since last good packet received from unit controller.
 *
 * Return 0xffffffff if unit controller is unset or never receved.
 */
uint32_t
sparrow_oam_get_time_since_last_good_rx_from_uc(void)
{
  uint64_t when;
  uint64_t elapsed;

  when = sparrow_oam_last_activity(SPARROW_OAM_ACTIVITY_TYPE_RX_FROM_UC);
  if(when == 0) {
    /* never received */
    return 0xffffffffL;
  }

  if(sparrow_uc_get_watchdog() == 0) {
    /* No unit controlelr */
    return 0xffffffffL;
  }

  elapsed = uptime_elapsed(when);
  if(elapsed > 0xffffffffLL) {
    return 0xffffffffL;
  }

  return (uint32_t)elapsed;
}
/*---------------------------------------------------------------------------*/

/*
 * Call oam notification in all instances that defined a notification handler.
 */
void
sparrow_oam_notify_all_instances(sparrow_oam_nt_t type, uint8_t operation)
{
  int i;

  for(i = 0; instances[i] != NULL; i++) {
    if(instances[i]->notification) {
      instances[i]->notification(type, operation);
    }
  }
}
/*---------------------------------------------------------------------------*/

/*
 * Call oam notification in all sub-instances that defined a notification handler.
 */
void
sparrow_oam_notify_all_sub_instances(sparrow_oam_nt_t type, uint8_t operation)
{
  int i;

  /* Call oam notification in all instances exception instance 0 */
  for(i = 1; instances[i] != NULL; i++) {
    if(instances[i]->notification) {
      instances[i]->notification(type, operation);
    }
  }
}
/*---------------------------------------------------------------------------*/

/*
 * OAM framework implementation.
 */

/*
 * Common processing of discovery variables for sub-instances.
 *
 * Process an OAM request placed on "request", no more than "len" bytes.
 * If needed, generate a reply into "reply", no more than "replymax" bytes.
 * Return number of bytes in "reply" to send or 0 on no reply to send.
 */
size_t
sparrow_oam_process_discovery_request(const sparrow_tlv_t *request, uint8_t *reply, size_t len, sparrow_oam_processing_t *oam_processing)
{
  const sparrow_oam_instance_t **local_ic = instances;
  if((request->opcode == SPARROW_TLV_OPCODE_SET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST)) {
    if(request->variable == VARIABLE_EVENT_ARRAY) {
      uint8_t cancelEvent = FALSE;
      if(request->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
        if(request->offset == 0) {
          // only LSB in first offset, enable/disable bit, is used.
          if((request->data[3] & 0x01) != (local_ic[request->instance]->data->event_array[3] & 0x01)) {
            local_ic[request->instance]->data->event_array[3] ^= 0x01;
          }
          if(request->elements > 1) {
            cancelEvent = sparrow_oam_do_event_action(&local_ic[request->instance]->data->event_array[7], request->data[7]);
          }
        } else {
          /* offset must be 1, since we would have been returning error otherwise. */
          cancelEvent = sparrow_oam_do_event_action(&local_ic[request->instance]->data->event_array[7], request->data[3]);
        }
        if(cancelEvent) {
          sparrow_var_update_event_arrays();
        }
        return sparrow_tlv_write_reply_vector(request, reply, len, local_ic[request->instance]->data->event_array);
      } else {
        if((request->data[3] & 0x01) != (local_ic[request->instance]->data->event_array[3] & 0x01)) {
          local_ic[request->instance]->data->event_array[3] ^= 0x01;
        }
        return sparrow_tlv_write_reply32(request, reply, len, local_ic[request->instance]->data->event_array);
      }
    } /* VARIABLE_EVENT_ARRAY */

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

  } else if((request->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST)) {
    if(request->variable == VARIABLE_OBJECT_TYPE) {
      return sparrow_tlv_write_reply64int(request, reply, len, local_ic[request->instance]->object_type);
    }
    if(request->variable == VARIABLE_OBJECT_ID) {
      if(local_ic[request->instance]->get_object_id) {
        const sparrow_id_t *id;
        id = local_ic[request->instance]->get_object_id(local_ic[request->instance]);
        if(id != NULL) {
          return sparrow_tlv_write_reply128(request, reply, len, id->id);
        }
      }
      return sparrow_tlv_write_reply128(request, reply, len, sparrow_tlv_zeroes);
    }
    if(request->variable == VARIABLE_OBJECT_LABEL) {
      return sparrow_tlv_write_reply256(request, reply, len, (const uint8_t *)local_ic[request->instance]->label);
    }
    if(request->variable == VARIABLE_NUMBER_OF_INSTANCES) {
      uint32_t i = local_ic[request->instance]->data->sub_instance_count;
      return sparrow_tlv_write_reply32int(request, reply, len, i);
    }
    if(request->variable == VARIABLE_OBJECT_SUB_TYPE) {
      if(local_ic[request->instance]->get_object_sub_type) {
        return sparrow_tlv_write_reply32int(request, reply, len, local_ic[request->instance]->get_object_sub_type(local_ic[request->instance]));

      }
    }
    if(request->variable == VARIABLE_EVENT_ARRAY) {
      if(request->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
        return sparrow_tlv_write_reply_vector(request, reply, len, local_ic[request->instance]->data->event_array);
      } else {
        return sparrow_tlv_write_reply32(request, reply, len, local_ic[request->instance]->data->event_array);
      }
    }

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

  }
  return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}
/*---------------------------------------------------------------------------*/
/*
 * Process an OAM request placed on "request", no more than "len" bytes.
 * If needed, generate a reply into "reply", no more than "replymax" bytes.
 * Return number of bytes in "reply" to send or 0 on no reply to send.
 */
size_t
sparrow_oam_process_request(const uint8_t *request, size_t len,
                            uint8_t *reply, size_t replymax,
                            sparrow_encap_pdu_info_t *pinfo)
{
  int reqpos;
  size_t reply_len = 0;
  int thislen = 0;
  sparrow_tlv_t T;
  sparrow_tlv_t *t = &T;
  sparrow_oam_processing_t oam_processing = SPARROW_OAM_PROCESSING_NEW;
  uint8_t error;
  uint8_t is_psp;

  if(replymax < 8) {
    return 0;
  }

  if(len <= 0) {
    return 0;
  }

  is_psp = (pinfo->payload_type == SPARROW_ENCAP_PAYLOAD_PORTAL_SELECTION_PROTO);

  reqpos = 0;
  sparrow_oam_pdu_begin();

  /* Process the TLV stack */
  for(reqpos = 0; (thislen = sparrow_tlv_from_bytes(t, request + reqpos)) > 0; reqpos += thislen) {

    /* first check end processing from last turn */
    if(oam_processing & SPARROW_OAM_PROCESSING_DO_NOT_REPLY) {
      pinfo->dont_reply = TRUE;
    }
    if(oam_processing & SPARROW_OAM_PROCESSING_ABORT_TLV_STACK) {
      break;
    }

    /*
     * If parsing this TLV would read outside buffer, abort stack parsing.
     */
    if((thislen + reqpos) > len) {
      DEBUG_PRINT_I("TLV @ %d parsing beyond buffer, aborting\n", reqpos);
      break;
    }

    /*
     * If this instance is handled elsewhere, give it there, without
     * any checks.
     */
    if((t->instance < INSTANCE_COUNT) &&
       (instances[t->instance]->data->process == SPARROW_OAM_INSTANCE_PROCESS_EXTERNAL)) {
      if(instances[t->instance]->process_request) {
        reply_len += instances[t->instance]->process_request(instances[t->instance], t, reply + reply_len, replymax - reply_len, &oam_processing);
      }
      continue;
    }

    /*
     * Reflector for any instance is handeled here.
     */
    if((t->opcode == SPARROW_TLV_OPCODE_REFLECT_REQUEST) || (t->opcode == SPARROW_TLV_OPCODE_VECTOR_REFLECT_REQUEST)) {
      sparrow_tlv_replyify(t);
      reply_len += sparrow_tlv_to_bytes(t, reply + reply_len, replymax - reply_len);
      continue;
    }

    /*
     * Process Event request
     */
    if((t->opcode == SPARROW_TLV_OPCODE_EVENT_REQUEST) || (t->opcode == SPARROW_TLV_OPCODE_VECTOR_EVENT_REQUEST)) {
      sparrow_tlv_replyify(t);
      reply_len += sparrow_tlv_to_bytes(t, reply + reply_len, replymax - reply_len);
      continue;
    }

    error = sparrow_var_check_tlv(t, is_psp);
    /* DEBUG_PRINT_II("@%04d: Checking TLV error: %d\n", reqpos, error); */
    if(error == SPARROW_TLV_ERROR_UNPROCESSED_TLV) {
      /* Silently skip it. */
      continue;
    }
    /* Silently skip unknown PSP variables */
    if(is_psp && (error == SPARROW_TLV_ERROR_UNKNOWN_VARIABLE)) {
      continue;
    }
    if(error != SPARROW_TLV_ERROR_NO_ERROR) {
      t->error = error;
      reply_len += sparrow_tlv_to_bytes(t, reply + reply_len, replymax - reply_len);
      continue;
    }

    /*
     * Process request
     */
    if(is_psp) {
      reply_len += sparrow_psp_process_request(t, reply + reply_len, replymax - reply_len, &oam_processing);
    } else if(sparrow_tlv_is_discovery_variable(t)) {
      reply_len += sparrow_oam_process_discovery_request(t, reply + reply_len, replymax - reply_len, &oam_processing);
    } else if(instances[t->instance]->process_request) {
      reply_len += instances[t->instance]->process_request(instances[t->instance], t, reply + reply_len, replymax - reply_len, &oam_processing);
    }

    if((oam_processing & SPARROW_OAM_PROCESSING_ABORT_TLV_STACK) || (oam_processing & SPARROW_OAM_PROCESSING_DO_NOT_REPLY)) {
      continue;
    }
  } /* for(reqpos = 0; (thislen = tlv_from_bytes(t, request + reqpos)) > 0; reqpos += thislen) */

  sparrow_oam_pdu_end();

  return reply_len;
}
/*---------------------------------------------------------------------------*/

/*
 * Called from interrupt when a new event happens.
 *
 * Return non-zero on command
 * SPARROW_OAM_EVENT_BACKOFF_TIMER_ARE_WE_THERE_YET if it is time to
 * transmit event.
 *
 * Return boottimer time when next event is due when called with command
 * SPARROW_OAM_EVENT_BACKOFF_TIMER_GET_NEXT_TX_TIME, och zero if timee is
 * not active.
 */
#define START_INCREMENT 250;
uint64_t
sparrow_oam_event_backoff_timer(sparrow_oam_event_backoff_timer_cmd_t cmd)
{
  static uint64_t next_event_tx = 0;
  static uint32_t increment;

  switch(cmd) {
  case SPARROW_OAM_EVENT_BACKOFF_TIMER_START:
    next_event_tx = uptime_read();
    increment = START_INCREMENT;
    process_poll(&oam);
    break;
  case SPARROW_OAM_EVENT_BACKOFF_TIMER_STOP:
    next_event_tx = 0;
    break;
  case SPARROW_OAM_EVENT_BACKOFF_TIMER_ARE_WE_THERE_YET:
    if((next_event_tx > 0) && (uptime_read() >= next_event_tx)) {
      next_event_tx += increment;
      if(increment < 32767) {
        increment *= 2;
      } else {
#ifdef STOP_EVENT_TRANSMIT_AT_MAXIMUM_INCREMENT
        next_event_tx = 0;
#endif /* STOP_EVENT_TRANSMIT_AT_MAXIMUM_INCREMENT */
      }
      return 1;
    }
    break;
  case SPARROW_OAM_EVENT_BACKOFF_TIMER_IS_ACTIVE:
    if(next_event_tx > 0) {
      return TRUE;
    } else {
      return FALSE;
    }
    break;
  case SPARROW_OAM_EVENT_BACKOFF_TIMER_GET_NEXT_TX_TIME:
    return next_event_tx;
    break;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/

uint8_t
sparrow_oam_is_event_triggered(const sparrow_oam_instance_t *instance)
{
  return instance != NULL && (instance->data->event_array[3] & SPARROW_OAM_EVENTSTATE_OFFSET0_TRIGGED_MASK) != 0;
}
/*---------------------------------------------------------------------------*/
/*
 * Trig event. Typically called from interrupt to set event state to
 * trigged, and start event transmission if events are enabled in
 * instance 0.
 */
void
sparrow_oam_trig_event(const sparrow_oam_instance_t *instance)
{
  const sparrow_oam_instance_t **local_ic = sparrow_oam_get_instances();
  if(instance == NULL) {
    return;
  }

  if(instance->data->event_array[7] == SPARROW_OAM_EVENTSTATE_EVENT_ARMED) {
    /* changed from armed to trigged */
    instance->data->event_array[7] = SPARROW_OAM_EVENTSTATE_EVENT_TRIGGED;
    instance->data->event_array[3] |= SPARROW_OAM_EVENTSTATE_OFFSET0_TRIGGED_MASK;
    local_ic[0]->data->event_array[3] |= SPARROW_OAM_EVENTSTATE_OFFSET0_TRIGGED_MASK;

    /* If both this instance and instance zero have events enabled then reset backoff timer */
    if((instance->data->event_array[3] & SPARROW_OAM_EVENTSTATE_OFFSET0_ENABLE_MASK) &&
        (local_ic[0]->data->event_array[3] & SPARROW_OAM_EVENTSTATE_OFFSET0_ENABLE_MASK)) {
      sparrow_oam_event_backoff_timer(SPARROW_OAM_EVENT_BACKOFF_TIMER_START);
    }
  }
}
/*---------------------------------------------------------------------------*/

/*
 * Write event TLV(s) to "reply", no more than "len" bytes.
 * Return number of bytes written.
 */
size_t
sparrow_oam_generate_event_reply(uint8_t *reply, size_t len)
{
  const sparrow_oam_instance_t **local_ic = instances;
  sparrow_tlv_t T;
  size_t retval = 0;
  int i;

  /* Boot timer read response */
  memset(&T, 0, sizeof (T));
  T.variable = VARIABLE_UNIT_BOOT_TIMER;
  T.opcode = SPARROW_TLV_OPCODE_GET_RESPONSE;
  retval += sparrow_tlv_write_reply64int(&T, reply, len, boottimerIEEE64());

#ifdef VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX
  memset(&T, 0, sizeof (T));
  T.variable = VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX;
  T.opcode = SPARROW_TLV_OPCODE_GET_RESPONSE;
  T.element_size = 4;
  retval += sparrow_tlv_write_reply32int(&T, &reply[retval], len - retval,
                                         sparrow_oam_get_time_since_last_good_rx_from_uc());
#endif /* VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX */

  for(i = 1; local_ic[i] != NULL; i++) {
    if(local_ic[i]->data->event_array[3] & SPARROW_OAM_EVENTSTATE_OFFSET0_TRIGGED_MASK) {
      T.instance = i;
      /* First add the instance' own event TLVs */
      if(local_ic[i]->poll) {
        retval += local_ic[i]->poll(local_ic[i], reply + retval, len - retval, SPARROW_OAM_POLL_TYPE_EVENT);
      }
      /* prepare and add the event TLV */
      memset(&T, 0, sizeof (T));
      T.length = 8;
      T.variable = VARIABLE_EVENT_ARRAY;
      T.opcode = SPARROW_TLV_OPCODE_EVENT_RESPONSE;
      T.element_size = 4;
      T.instance = i;
      retval += sparrow_tlv_to_bytes(&T, reply + retval, len - retval);
    }
  }
  return retval;
}
/*---------------------------------------------------------------------------*/
/**
 * Update event state with action.
 * Return TRUE if a trigged event was acknowledged.
 */
uint8_t
sparrow_oam_do_event_action(uint8_t *state, uint8_t action)
{
  uint8_t new_state = *state;
  uint8_t retval = FALSE;

  switch(*state) {
  case SPARROW_OAM_EVENTSTATE_EVENT_DISARMED:
    if(action == SPARROW_OAM_EVENTACTION_EVENT_ARM) {
      new_state = SPARROW_OAM_EVENTSTATE_EVENT_ARMED;
    } else if(action == SPARROW_OAM_EVENTACTION_EVENT_REARM) {
      new_state = SPARROW_OAM_EVENTSTATE_EVENT_ARMED;
    }
    break;
  case SPARROW_OAM_EVENTSTATE_EVENT_ARMED:
    if(action == SPARROW_OAM_EVENTACTION_EVENT_DISARM) {
      new_state = SPARROW_OAM_EVENTSTATE_EVENT_DISARMED;
    }
    break;
  case SPARROW_OAM_EVENTSTATE_EVENT_TRIGGED:
    if(action == SPARROW_OAM_EVENTACTION_EVENT_REARM) {
      new_state = SPARROW_OAM_EVENTSTATE_EVENT_ARMED;
      retval = TRUE;
    } else if(action == SPARROW_OAM_EVENTACTION_EVENT_DISARM) {
      new_state = SPARROW_OAM_EVENTSTATE_EVENT_DISARMED;
      retval = TRUE;
    }
    break;
  }

  *state = new_state;
  return retval;
}
/*---------------------------------------------------------------------------*/
