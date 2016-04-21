/*
 * Copyright (c) 2012-2016, Yanzi Networks AB.
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
 *         Routing table instance.
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

/**
 *
 * RPL routing table, implements object 0x0090DA0302010015.
 *
 */
#include <stdlib.h>
#include "sparrow-oam.h"
#include "instance-rtable.h"
#include "instance-rtable-var.h"

#include "net/ipv6/uip-ds6.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define YLOG_LEVEL YLOG_LEVEL_NONE
#define YLOG_NAME  "rtable"
#include "ylog.h"

#if ! UIP_DS6_NOTIFICATIONS
#error "The routing instance requires that UIP DS6 notifications are enabled."
#endif /* ! UIP_DS6_NOTIFICATIONS */

static rtable_entry_t *local_table = NULL;
static uint32_t route_table_revision = 0;
static uint32_t local_table_revision = 0;
static uint32_t table_length;

static struct uip_ds6_notification route_notification;
/*----------------------------------------------------------------*/
static void
route_callback(int event, uip_ipaddr_t *route, uip_ipaddr_t *nexthop,
               int num_routes)
{
  switch(event) {
  case UIP_DS6_NOTIFICATION_DEFRT_ADD:
    if(YLOG_IS_LEVEL(YLOG_LEVEL_DEBUG)) {
      YLOG_DEBUG("route: added defrt ");
      uip_debug_ipaddr_print(route);
      printf("\n");
    }
    break;

  case UIP_DS6_NOTIFICATION_DEFRT_RM:
    if(YLOG_IS_LEVEL(YLOG_LEVEL_DEBUG)) {
      YLOG_DEBUG("route: removed defrt ");
      uip_debug_ipaddr_print(route);
      printf("\n");
    }
    break;

  case UIP_DS6_NOTIFICATION_ROUTE_ADD:
    route_table_revision++;

    if(YLOG_IS_LEVEL(YLOG_LEVEL_DEBUG)) {
      YLOG_DEBUG("route: added ");
      uip_debug_ipaddr_print(route);
      printf(" via ");
      uip_debug_ipaddr_print(nexthop);
      printf(" (now %d routes)\n", num_routes);
    }
    break;

  case UIP_DS6_NOTIFICATION_ROUTE_RM :
    route_table_revision++;

    if(YLOG_IS_LEVEL(YLOG_LEVEL_DEBUG)) {
      YLOG_DEBUG("route: removed ");
      uip_debug_ipaddr_print(route);
      printf(" (now %d routes)\n", num_routes);
    }
    break;
  }
}
/*----------------------------------------------------------------*/
static void
update_local_table(void)
{
  rtable_entry_t *old_table;
  rtable_entry_t *new_table;
  int new_size;
  int n;
  uip_ds6_route_t *r;
  uip_ipaddr_t *nexthop;

  if(route_table_revision == local_table_revision) {
    /* No table update necessary */

    /* TODO Add and update route metrics */
    return;
  }

  new_size = uip_ds6_route_num_routes();
  new_table = malloc(new_size * sizeof (rtable_entry_t));
  if(new_table == NULL) {
    YLOG_ERROR("Failed to alloc new table.\n");
    return;
  }
  memset(new_table, 0, new_size * sizeof (rtable_entry_t));

  n = 0;
  for(r = uip_ds6_route_head() ; r != NULL ; r = uip_ds6_route_next(r)) { // loop over routes
    memcpy(new_table[n].address, &r->ipaddr.u8, 16);
    nexthop = uip_ds6_route_nexthop(r);
    if(nexthop != NULL) {
      memcpy(new_table[n].nexthop, nexthop, 16);
    } else {
      memset(new_table[n].nexthop, 0, 16);
    }
    new_table[n].length = r->length;
    /* TODO: add metric! */
    new_table[n].metric = 0;
    // XXX: parse state for flags
    n++;
  }

  if(table_length != new_size) {
    YLOG_DEBUG("old routing table size is %d new size id %d, installing new table\n", table_length, new_size);
  }

  /* install new table */
  old_table = local_table;
  local_table = new_table;
  table_length = new_size;
  local_table_revision = route_table_revision;
  free(old_table);
  YLOG_DEBUG("Installed table revision %d, length %d\n", local_table_revision, table_length);
}
/*----------------------------------------------------------------*/
/**
 * Process a request TLV.
 *
 * Writes a response TLV starting at "reply", no more than "len"
 * bytes.
 *
 * if the "request" is of type vectorAbortIfEqualRequest or
 * abortIfEqualRequest, and processing should be aborted,
 * "oam_processing" is set to SPARROW_OAM_PROCESSING_ABORT_TLV_STACK.
 *
 * Returns number of bytes written to "reply".
 */
static size_t
rtable_process_request(const sparrow_oam_instance_t *instance,
                       sparrow_tlv_t *request, uint8_t *reply, size_t len,
                       sparrow_oam_processing_t *oam_processing)
{
  int i;

  if((request->opcode == SPARROW_TLV_OPCODE_SET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST)) {
    /*
     * Payload variables
     */
    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  } else if((request->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST)) {
    /*
     * Payload variables
     */

    if(request->variable == VARIABLE_TABLE_REVISION) {
      update_local_table();
      return sparrow_tlv_write_reply32int(request, reply, len, local_table_revision);
    }

    if(request->variable == VARIABLE_NETWORK_ADDRESS) {
      for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
        if(uip_ds6_if.addr_list[i].isused && (uip_ds6_if.addr_list[i].state == ADDR_PREFERRED)) {
          return sparrow_tlv_write_reply128(request, reply, len,(uint8_t *) &(uip_ds6_if.addr_list[i]).ipaddr);
        }
      }
      return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
    }
    if(request->variable == VARIABLE_TABLE_LENGTH) {
      update_local_table();
      return sparrow_tlv_write_reply32int(request, reply, len, table_length);
    }

    if(request->variable == VARIABLE_TABLE) {
      if(request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST) {
        update_local_table();
        if(request->offset > table_length) {
          request->elements = 0;
        } else {
          request->elements = MIN(request->elements, (table_length - request->offset));
        }
        return sparrow_tlv_write_reply_vector(request, reply, len, ((uint8_t *)local_table));

      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_NO_VECTOR_ACCESS, reply, len);
      }
    }

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }
  return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}
/*----------------------------------------------------------------*/

/*
 * Init called from sparrow_oam_init.
 */
static void
init(const sparrow_oam_instance_t *instance)
{
  instance->data->event_array[1] = 1;
  uip_ds6_notification_add(&route_notification, route_callback);
}
/*----------------------------------------------------------------*/
SPARROW_OAM_INSTANCE(instance_rtable,
                     INSTANCE_RTABLE_OBJECT_TYPE, INSTANCE_RTABLE_LABEL,
                     instance_rtable_variables,
                     .init = init,
                     .process_request = rtable_process_request);
/*---------------------------------------------------------------------------*/
