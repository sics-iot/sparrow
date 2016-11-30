/*
 * Copyright (c) 2013-2016, Yanzi Networks AB.
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
 *         Instance radio implementation for serial-radio.
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 */
#include "contiki.h"
#include "dev/sparrow-device.h"
#include "sparrow-oam.h"
#include "boot-data.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/mac/handler-802154.h"
#include "cmd.h"
#include "serial-radio.h"
#include "serial-radio-stats.h"
#include "radio-scan.h"
#include "radio-frontpanel.h"
#include "sparrow-beacon.h"
#include <stdio.h>
#include "instance-radio-var.h"

uint32_t serial_radio_stats[SERIAL_RADIO_STATS_MAX];
uint32_t serial_radio_stats_debug[SERIAL_RADIO_STATS_DEBUG_MAX];
static uint16_t reset_info = 0;

#ifdef WITH_868
#define HIGHEST_RADIO_CHANNEL 33
#define LOWEST_CHANNEL 0
#elif defined(WITH_920)
#define HIGHEST_RADIO_CHANNEL 37
#define LOWEST_CHANNEL 0
#else
#define HIGHEST_RADIO_CHANNEL 26
#define LOWEST_CHANNEL 11
#endif

/*----------------------------------------------------------------*/
uint32_t
radio_get_reset_cause(void)
{
  return reset_info;
}
/*----------------------------------------------------------------*/
/**
 * Process a request TLV.
 *
 * Writes a response TLV starting at "reply", ni more than "len"
 * bytes.
 *
 * if the "request" is of type vectorAbortIfEqualRequest or
 * abortIfEqualRequest, and processing should be aborted,
 * "oam_processing" is set to SPARROW_OAM_PROCESSING_ABORT_TLV_STACK.
 *
 * Returns number of bytes written to "reply".
 */
static size_t
radio_process_request(const sparrow_oam_instance_t *instance,
                      sparrow_tlv_t *request, uint8_t *reply, size_t len,
                      sparrow_oam_processing_t *oam_processing)
{
  uint32_t local32;
  if((request->opcode == SPARROW_TLV_OPCODE_SET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST)) {

#ifdef VARIABLE_RADIO_CHANNEL
    if(request->variable == VARIABLE_RADIO_CHANNEL) {
      local32 = sparrow_tlv_get_int32_from_data(request->data);
      if((local32 <= HIGHEST_RADIO_CHANNEL) && (local32 >= LOWEST_CHANNEL)) {
        serial_radio_set_channel(local32);
        return sparrow_tlv_write_reply32(request, reply, len, 0);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
    }
#endif /* VARIABLE_RADIO_CHANNEL */

#ifdef VARIABLE_RADIO_PAN_ID
    if(request->variable == VARIABLE_RADIO_PAN_ID) {
      uint16_t pan_id = request->data[2] << 8 | request->data[3];
      serial_radio_set_pan_id(pan_id, 1);
      return sparrow_tlv_write_reply32(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_PAN_ID */

#ifdef VARIABLE_RADIO_BEACON_RESPONSE
    if(request->variable == VARIABLE_RADIO_BEACON_RESPONSE) {
      int i;
      /* Validate beacon response type and basic length */
      if(request->data[0] != 0xfe) {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      if(request->element_size * request->elements > HANDLER_802154_BEACON_PAYLOAD_BUFFER_SIZE) {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      i = sparrow_beacon_calculate_payload_length(request->data, request->element_size * request->elements);
      if(i > 0) {
        handler_802154_set_beacon_payload(request->data, i);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      printf("radio: setting beacon : ");
      for(local32 = 0; local32 < i; local32++) {
        printf("%02x", request->data[local32]);
      }
      printf("\n");
      {
        /* Tell our border router */
        uint8_t buf[HANDLER_802154_BEACON_PAYLOAD_BUFFER_SIZE + 4];
        buf[0] = '!';
        buf[1] = 'B';
        buf[2] = i;
        memcpy(&buf[3], request->data, i);
        cmd_send(buf, i + 3);
      }
      return sparrow_tlv_write_reply_vector(request, reply, len, sparrow_tlv_zeroes);
    }
#endif /* VARIABLE_RADIO_BEACON_RESPONSE */

#ifdef VARIABLE_RADIO_MODE
    if(request->variable == VARIABLE_RADIO_MODE) {
      local32 = sparrow_tlv_get_int32_from_data(request->data);
      radio_set_mode(local32);
      {
        /* Tell our border router */
        uint8_t buf[4];
        buf[0] = '!';
        buf[1] = 'm';
        buf[2] = radio_get_mode() & 0xff;
        cmd_send(buf, 3);
      }
      return sparrow_tlv_write_reply32(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_MODE */

#ifdef VARIABLE_RADIO_SCAN_MODE
    if(request->variable == VARIABLE_RADIO_SCAN_MODE) {
      local32 = sparrow_tlv_get_int32_from_data(request->data);
      radio_scan_set_mode(local32);
      return sparrow_tlv_write_reply32(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_SCAN_MODE */

#ifdef VARIABLE_RADIO_SERIAL_MODE
    if(request->variable == VARIABLE_RADIO_SERIAL_MODE) {
      local32 = sparrow_tlv_get_int32_from_data(request->data);
      serial_set_mode(local32);
      return sparrow_tlv_write_reply32(request, reply, len, 0);
    }
#endif /* VARIABLE_RADIO_SERIAL_MODE */

#ifdef HAVE_RADIO_FRONTPANEL

 #ifdef VARIABLE_RADIO_WATCHDOG
    if(request->variable == VARIABLE_RADIO_WATCHDOG) {
      local32 = sparrow_tlv_get_int32_from_data(request->data);
      radio_frontpanel_set_watchdog(local32);
      return sparrow_tlv_write_reply32int(request, reply, len, 0);
    }
 #endif /* VARIABLE_RADIO_WATCHDOG */

 #ifdef VARIABLE_RADIO_FRONTPANEL_INFO
    if(request->variable == VARIABLE_RADIO_FRONTPANEL_INFO) {
      local32 = sparrow_tlv_get_int32_from_data(request->data);
      radio_frontpanel_set_info(local32);
      return sparrow_tlv_write_reply32int(request, reply, len, 0);
    }
 #endif /* VARIABLE_RADIO_FRONTPANEL_INFO */

 #ifdef VARIABLE_RADIO_FRONTPANEL_ERROR_CODE
    if(request->variable == VARIABLE_RADIO_FRONTPANEL_ERROR_CODE) {
      local32 = sparrow_tlv_get_int32_from_data(request->data);
      radio_frontpanel_set_error_code(local32);
      return sparrow_tlv_write_reply32int(request, reply, len, 0);
    }
 #endif /* VARIABLE_RADIO_FRONTPANEL_INFO */

#endif /* HAVE_RADIO_FRONTPANEL */

#ifdef VARIABLE_RADIO_RESET_CAUSE
    if(request->variable == VARIABLE_RADIO_RESET_CAUSE) {
      /* Ignore the incoming value - just clear the reset cause */
      reset_info = (RADIO_RESET_CAUSE_NONE << 8) | RADIO_EXT_RESET_CAUSE_NONE;
      local32 = 0;
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_RESET_CAUSE */

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }

  if((request->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST)) {

#ifdef VARIABLE_RADIO_CHANNEL
    if(request->variable == VARIABLE_RADIO_CHANNEL) {
      radio_value_t channel;
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &channel) == RADIO_RESULT_OK) {
        return sparrow_tlv_write_reply32int(request, reply, len, channel);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }
#endif /* VARIABLE_RADIO_CHANNEL */

#ifdef VARIABLE_RADIO_PAN_ID
    if(request->variable == VARIABLE_RADIO_PAN_ID) {
      radio_value_t panid;
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_PAN_ID, &panid) == RADIO_RESULT_OK) {
        return sparrow_tlv_write_reply32int(request, reply, len, panid);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }
#endif /* VARIABLE_RADIO_PAN_ID */

#ifdef VARIABLE_RADIO_BEACON_RESPONSE
    if(request->variable == VARIABLE_RADIO_BEACON_RESPONSE) {
      uint8_t *beacon_payload;
      uint8_t beacon_payload_len;
      beacon_payload = handler_802154_get_beacon_payload(&beacon_payload_len);
      request->elements = beacon_payload_len / request->element_size;
      if(beacon_payload_len % request->element_size) {
        request->elements++;
      }
      return sparrow_tlv_write_reply_vector(request, reply, len, beacon_payload);
    }
#endif /* VARIABLE_RADIO_BEACON_RESPONSE */

#ifdef VARIABLE_RADIO_MODE
    if(request->variable == VARIABLE_RADIO_MODE) {
      local32 = radio_get_mode();
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_MODE */

#ifdef VARIABLE_RADIO_SCAN_MODE
    if(request->variable == VARIABLE_RADIO_SCAN_MODE) {
      local32 = radio_scan_get_mode();
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_SCAN_MODE */

#ifdef VARIABLE_RADIO_SERIAL_MODE
    if(request->variable == VARIABLE_RADIO_SERIAL_MODE) {
      local32 = serial_get_mode();
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_SERIAL_MODE */

#ifdef VARIABLE_RADIO_STAT_LENGTH
    if(request->variable == VARIABLE_RADIO_STAT_LENGTH) {
      local32 = SERIAL_RADIO_STATS_MAX;
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_STAT_LENGTH */

#ifdef VARIABLE_RADIO_STAT_DATA
    if(request->variable == VARIABLE_RADIO_STAT_DATA) {
      if(request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST) {
        if(request->offset >= SERIAL_RADIO_STATS_MAX) {
          request->elements = 0;
        } else {
          request->elements = MIN(request->elements, (SERIAL_RADIO_STATS_MAX - request->offset));
        }
        return sparrow_tlv_write_reply_vector(request, reply, len, ((uint8_t *)serial_radio_stats));
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_NO_VECTOR_ACCESS, reply, len);
      }
    }
#endif /* VARIABLE_RADIO_STAT_DATA */

#ifdef VARIABLE_RADIO_CONTROL_API_VERSION
    if(request->variable == VARIABLE_RADIO_CONTROL_API_VERSION) {
      local32 = SERIAL_RADIO_CONTROL_API_VERSION;
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_CONTROL_API_VERSION */

#ifdef HAVE_RADIO_FRONTPANEL

#ifdef VARIABLE_RADIO_SUPPLY_STATUS
    if(request->variable == VARIABLE_RADIO_SUPPLY_STATUS) {
      local32 = radio_frontpanel_get_supply_status();
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_SUPPLY_STATUS */

#ifdef VARIABLE_RADIO_BATTERY_SUPPLY_TIME
    if(request->variable == VARIABLE_RADIO_BATTERY_SUPPLY_TIME) {
      local32 = radio_frontpanel_get_battery_supply_time();
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_BATTERY_SUPPLY_TIME */

#ifdef VARIABLE_RADIO_SUPPLY_VOLTAGE
    if(request->variable == VARIABLE_RADIO_SUPPLY_VOLTAGE) {
      local32 = radio_frontpanel_get_supply_voltage();
      if(local32 != 0xffffffffL) {
        return sparrow_tlv_write_reply32int(request, reply, len, local32);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }
#endif /* VARIABLE_RADIO_SUPPLY_VOLTAGE */

#ifdef VARIABLE_RADIO_WATCHDOG
    if(request->variable == VARIABLE_RADIO_WATCHDOG) {
      local32 = radio_frontpanel_get_watchdog();
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_WATCHDOG */

#ifdef VARIABLE_RADIO_FRONTPANEL_INFO
    if(request->variable == VARIABLE_RADIO_FRONTPANEL_INFO) {
      local32 = radio_frontpanel_get_info();
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_FRONTPANEL_INFO */

#ifdef VARIABLE_RADIO_FRONTPANEL_ERROR_CODE
    if(request->variable == VARIABLE_RADIO_FRONTPANEL_ERROR_CODE) {
      local32 = radio_frontpanel_get_error_code();
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_FRONTPANEL_ERROR_CODE */

#endif /* HAVE_RADIO_FRONTPANEL */

#ifdef VARIABLE_RADIO_RESET_CAUSE
    if(request->variable == VARIABLE_RADIO_RESET_CAUSE) {
      local32 = reset_info;
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_RESET_CAUSE */

    /* Debug variables */

#ifdef VARIABLE_RADIO_UNIT_BOOT_TIMER
    if(request->variable == VARIABLE_RADIO_UNIT_BOOT_TIMER) {
      if(request->element_size == 8) {
        return sparrow_tlv_write_reply64int(request, reply, len, boottimerIEEE64());
      }
      if(request->element_size == 4) {
        return sparrow_tlv_write_reply32int(request, reply, len, boottimerIEEE64in32());
      }
      return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_ELEMENT_SIZE, reply, len);
    }
#endif /* VARIABLE_RADIO_UNIT_BOOT_TIMER */

#ifdef VARIABLE_RADIO_STAT_DEBUG_LENGTH
    if(request->variable == VARIABLE_RADIO_STAT_DEBUG_LENGTH) {
      local32 = SERIAL_RADIO_STATS_DEBUG_MAX;
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_RADIO_STAT_DEBUG_LENGTH */

#ifdef VARIABLE_RADIO_STAT_DEBUG_DATA
    if(request->variable == VARIABLE_RADIO_STAT_DEBUG_DATA) {
      if(request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST) {
        if(request->offset >= SERIAL_RADIO_STATS_DEBUG_MAX) {
          request->elements = 0;
        } else {
          request->elements = MIN(request->elements, (SERIAL_RADIO_STATS_DEBUG_MAX - request->offset));
        }
        return sparrow_tlv_write_reply_vector(request, reply, len, ((uint8_t *)serial_radio_stats_debug));
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_NO_VECTOR_ACCESS, reply, len);
      }
    }
#endif /* VARIABLE_RADIO_STAT_DEBUG_DATA */

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }

  return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}
/*---------------------------------------------------------------------------*/
static void
init(const sparrow_oam_instance_t *instance)
{
  instance->data->event_array[1] = 1;

  if(reset_info == 0) {
    reset_info = SPARROW_DEVICE.get_reset_cause();
    reset_info <<= 8;

#ifdef HAVE_BOOT_DATA
    {
      boot_data_t *boot_data = boot_data_get();
      if(boot_data != NULL) {
        reset_info |= boot_data->extended_reset_cause & 0xff;

        /* Clear the reset cause for next reboot */
        boot_data->extended_reset_cause =
          SPARROW_DEVICE_EXTENDED_RESET_CAUSE_NORMAL;
      }
    }
#endif /* HAVE_BOOT_DATA */
  }
}
/*---------------------------------------------------------------------------*/
SPARROW_OAM_INSTANCE(instance_radio,
                     INSTANCE_RADIO_OBJECT_TYPE, INSTANCE_RADIO_LABEL,
                     instance_radio_variables,
                     .init = init,
                     .process_request = radio_process_request);
/*---------------------------------------------------------------------------*/
