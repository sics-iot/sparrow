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
 *
 *
 */

/**
 * \file
 *         Instance zero implementation.
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Oriol Piñol Piñol <oriol.pinol@yanzinetworks.com>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki-conf.h"
#include "sparrow-oam.h"
#include "sparrow-uc.h"
#include "sparrow-var.h"
#include "dev/sparrow-device.h"
#include "instance0-var.h"

#ifndef PRODUCT_TYPE_INT64
#error Please specify PRODUCT_TYPE_INT64 in project-conf.h
#endif /* PRODUCT_TYPE_INT64 */

#ifndef PRODUCT_LABEL
#error Please specify PRODUCT_LABEL in project-conf.h
#endif /* PRODUCT_LABEL */

#ifndef SPARROW_DEVICE
#error Please specify SPARROW_DEVICE for this platform.
#endif /* SPARROW_DEVICE */

#ifdef VARIABLE_TLV_GROUPING
extern uint64_t tlv_grouping_time;
extern uint32_t tlv_grouping_holdoff;
#endif /* VARIABLE_TLV_GROUPING */

#define FACTORY_DEFAULT_STATE_0 0
#define FACTORY_DEFAULT_STATE_1 1
#define FACTORY_DEFAULT_KEY_0 0x567190
#define FACTORY_DEFAULT_KEY_1 0x5

static uint8_t factory_default_state = FACTORY_DEFAULT_STATE_0;
/*---------------------------------------------------------------------------*/

/*
 * Instance 0 implementation
 */
static size_t
process0_request(const sparrow_oam_instance_t *instance,
                 sparrow_tlv_t *request, uint8_t *reply, size_t len,
                 sparrow_oam_processing_t *oam_processing)
{
  uint8_t error;

  if((request->opcode == SPARROW_TLV_OPCODE_SET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST)) {

#ifdef VARIABLE_UNIT_CONTROLLER_ADDRESS
    if(request->variable == VARIABLE_UNIT_CONTROLLER_ADDRESS) {
      error = sparrow_uc_set_address(request->data, FALSE);
      if(error == SPARROW_TLV_ERROR_NO_ERROR) {
        return sparrow_tlv_write_reply256(request, reply, len, 0);
      } else {
        return sparrow_tlv_write_reply_error(request, error, reply, len);
      }
    }
#endif /* VARIABLE_UNIT_CONTROLLER_ADDRESS */
#ifdef VARIABLE_UNIT_CONTROLLER_WATCHDOG
    if(request->variable == VARIABLE_UNIT_CONTROLLER_WATCHDOG) {
      uint32_t local32 = sparrow_tlv_get_int32_from_data(request->data);
      error = sparrow_uc_set_watchdog(local32);
      if(error == SPARROW_TLV_ERROR_NO_ERROR) {
        return sparrow_tlv_write_reply32(request, reply, len, 0);
      } else {
        return sparrow_tlv_write_reply_error(request, error, reply, len);
      }
    }
#endif /* VARIABLE_UNIT_CONTROLLER_WATCHDOG */
#ifdef VARIABLE_HARDWARE_RESET
    if(request->variable == VARIABLE_HARDWARE_RESET) {
      if((request->data[0] * 256 + request->data[1]) == 911) {
        SPARROW_DEVICE.reboot_to_image(request->data[3]);
        return sparrow_tlv_write_reply32(request, reply, len, 0);
      } else if(request->data[3] != 0) {
        SPARROW_DEVICE.reboot();
        /* Probably not reached */
        return sparrow_tlv_write_reply32(request, reply, len, 0);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
    }
#endif /* VARIABLE_HARDWARE_RESET */
#ifdef VARIABLE_TLV_GROUPING
    /*
     * Data format:
     *
     *   3               2               1
     *   1               3               5               7             0
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |   reply flag  |    reserved   |          holdoff ms           |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    if(request->variable == VARIABLE_TLV_GROUPING) {
      /* first clear hold-off time and call sparrow_oam_pdu_begin to clear state */
      tlv_grouping_holdoff = 0;
      sparrow_oam_pdu_begin();
      /* Then set new hold-off time */
      tlv_grouping_holdoff = (request->data[2] << 8) + request->data[3];
      tlv_grouping_time = uptime_read();

      if(request->data[0] == 0) {
        return 0;
      }
      return sparrow_tlv_write_reply32(request, reply, len, 0);
    }
#endif /* VARIABLE_TLV_GROUPING */
#ifdef VARIABLE_LOCATION_ID
    if(request->variable == VARIABLE_LOCATION_ID) {
      uint32_t local32 = sparrow_tlv_get_int32_from_data(request->data);
      if(local32 == 0) {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      if(sparrow_set_location_id(local32)) {
        return sparrow_tlv_write_reply32(request, reply, len, 0);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }
#endif /* VARIABLE_LOCATION_ID */
#ifdef VARIABLE_OWNER_ID
    if(request->variable == VARIABLE_OWNER_ID) {
      uint32_t local32 = sparrow_tlv_get_int32_from_data(request->data);
      if(local32 == 0) {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      if(sparrow_set_owner_id(local32)) {
        return sparrow_tlv_write_reply32(request, reply, len, 0);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }
#endif /* VARIABLE_OWNER_ID */
#ifdef VARIABLE_CHASSIS_FACTORY_DEFAULT
    if(request->variable == VARIABLE_CHASSIS_FACTORY_DEFAULT) {
      switch(factory_default_state) {
      case FACTORY_DEFAULT_STATE_0:
        if(sparrow_tlv_get_int32_from_data(request->data) == FACTORY_DEFAULT_KEY_0) {
          factory_default_state = FACTORY_DEFAULT_STATE_1;
        } else {
          return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_INVALID_ARGUMENT, reply, len);
        }
        ;;
      case FACTORY_DEFAULT_STATE_1:
        if(sparrow_tlv_get_int32_from_data(request->data) == FACTORY_DEFAULT_KEY_1) {
          /* erase and rewind */
          sparrow_set_factory_default();
          /* Notify all sub instances about the factory default */
          sparrow_oam_notify_all_sub_instances(SPARROW_OAM_NT_FACTORY_DEFAULT, 0);
          SPARROW_DEVICE.reboot();
          /* Probably not reached */
          return sparrow_tlv_write_reply32(request, reply, len, 0);
        } else {
          return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_INVALID_ARGUMENT, reply, len);
        }
        ;;
      default:
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_INVALID_ARGUMENT, reply, len);
        ;;
      } // switch state
    }
#endif /* VARIABLE_CHASSIS_FACTORY_DEFAULT */
#ifdef VARIABLE_IDENTIFY_CHASSIS
    if(request->variable == VARIABLE_IDENTIFY_CHASSIS) {
      if(SPARROW_DEVICE.beep) {
        SPARROW_DEVICE.beep(sparrow_tlv_get_int32_from_data(request->data));
      }
      return sparrow_tlv_write_reply32(request, reply, len, 0);
    }
#endif /* VARIABLE_IDENTIFY_CHASSIS  */

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

  } else if((request->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST)) {

#ifdef VARIABLE_UNIT_BOOT_TIMER
    if(request->variable == VARIABLE_UNIT_BOOT_TIMER) {
      if(request->element_size == 8) {
        return sparrow_tlv_write_reply64int(request, reply, len, boottimerIEEE64());
      }
      if(request->element_size == 4) {
        return sparrow_tlv_write_reply32int(request, reply, len, boottimerIEEE64in32());
      }
      return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_ELEMENT_SIZE, reply, len);
    }
#endif /* VARIABLE_UNIT_BOOT_TIMER */
#ifdef VARIABLE_SW_REVISION
    if(request->variable == VARIABLE_SW_REVISION) {
      return sparrow_tlv_write_reply128(request, reply, len,  sparrow_getswrevision());
    }
#endif /* VARIABLE_SW_REVISION */
#ifdef VARIABLE_REVISION
    if(request->variable == VARIABLE_REVISION) {
      return sparrow_tlv_write_reply128(request, reply, len,  sparrow_getswrevision());
    }
#endif /* VARIABLE_REVISION */
#ifdef VARIABLE_READ_THIS_COUNTER
    if(request->variable == VARIABLE_READ_THIS_COUNTER) {
      static uint32_t read_this_counter = 0;
      return sparrow_tlv_write_reply32int(request, reply, len,  read_this_counter++);
    }
#endif /* VARIABLE_READ_THIS_COUNTER */
#ifdef VARIABLE_UNIT_CONTROLLER_ADDRESS
    if(request->variable == VARIABLE_UNIT_CONTROLLER_ADDRESS) {
      return sparrow_tlv_write_reply256(request, reply, len, sparrow_uc_get_address());
    }
#endif /* VARIABLE_UNIT_CONTROLLER_ADDRESS */
#ifdef VARIABLE_UNIT_CONTROLLER_WATCHDOG
    if(request->variable == VARIABLE_UNIT_CONTROLLER_WATCHDOG) {
      return sparrow_tlv_write_reply32int(request, reply, len, sparrow_uc_get_watchdog());
    }
#endif /* VARIABLE_UNIT_CONTROLLER_WATCHDOG */
#ifdef VARIABLE_UNIT_CONTROLLER_STATUS
    if(request->variable == VARIABLE_UNIT_CONTROLLER_STATUS) {
      return sparrow_tlv_write_reply32int(request, reply, len, sparrow_uc_get_status());
    }
#endif /* VARIABLE_UNIT_CONTROLLER_STATUS */
#ifdef VARIABLE_COMMUNICATION_STYLE
    if(request->variable == VARIABLE_COMMUNICATION_STYLE) {
      return sparrow_tlv_write_reply32int(request, reply, len,
                                sparrow_uc_get_communication_style());
    }
#endif /* VARIABLE_COMMUNICATION_STYLE */
#ifdef VARIABLE_TLV_GROUPING
    if(request->variable == VARIABLE_TLV_GROUPING) {
      uint32_t local32 = uptime_elapsed(tlv_grouping_time);
      if(tlv_grouping_holdoff > local32) {
        return sparrow_tlv_write_reply32int(request, reply, len, tlv_grouping_holdoff - local32);
      } else {
        return sparrow_tlv_write_reply32int(request, reply, len, 0);
      }
    }
#endif /* VARIABLE_TLV_GROUPING */
#ifdef VARIABLE_LOCATION_ID
    if(request->variable == VARIABLE_LOCATION_ID) {
      uint32_t local32 = sparrow_get_location_id();
      if(local32 == 0) {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_LOCATION_ID */
#ifdef VARIABLE_OWNER_ID
    if(request->variable == VARIABLE_OWNER_ID) {
      uint32_t local32 = sparrow_get_owner_id();
      if(local32 == 0) {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
#endif /* VARIABLE_OWNER_ID */
#ifdef VARIABLE_CHASSIS_CAPABILITIES
    if(request->variable == VARIABLE_CHASSIS_CAPABILITIES) {
      uint64_t local64;
      if(SPARROW_DEVICE.get_capabilities &&
         SPARROW_DEVICE.get_capabilities(&local64)) {
        return sparrow_tlv_write_reply64int(request, reply, len, local64);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }
#endif /* VARIABLE_CHASSIS_CAPABILITIES */
#ifdef VARIABLE_CHASSIS_BOOTLOADER_VERSION
    if(request->variable == VARIABLE_CHASSIS_BOOTLOADER_VERSION) {
      uint32_t version;
      if(SPARROW_DEVICE.get_bootloader_version) {
        version = SPARROW_DEVICE.get_bootloader_version();
      } else {
        version = 0;
      }
      return sparrow_tlv_write_reply32int(request, reply, len, version);
    }
#endif /* VARIABLE_CHASSIS_BOOTLOADER_VERSION */

#ifdef VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX
    if(request->variable == VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX) {
      return sparrow_tlv_write_reply32int(request, reply, len, sparrow_oam_get_time_since_last_good_rx_from_uc());
    }
#endif /* VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX */

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }
  return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}
/*---------------------------------------------------------------------------*/
static size_t
poll0(const sparrow_oam_instance_t *instance, uint8_t *reply, size_t len,
      sparrow_oam_poll_type_t poll_type)
{
  size_t reply_len = 0;
  sparrow_tlv_t T;
  sparrow_oam_processing_t oam_processing = SPARROW_OAM_PROCESSING_NEW;

  T.version = SPARROW_TLV_VERSION;
  T.instance = sparrow_oam_get_instance_id(instance);
  T.opcode = SPARROW_TLV_OPCODE_GET_REQUEST;
  T.error = SPARROW_TLV_ERROR_NO_ERROR;
  T.variable = VARIABLE_OBJECT_TYPE;
  T.element_size = 8;

  reply_len += sparrow_oam_process_discovery_request(&T, reply, len, &oam_processing);

  T.variable = VARIABLE_OBJECT_ID;
  T.element_size = 16;
  reply_len += sparrow_tlv_write_reply128(&T, &reply[reply_len], len - reply_len, sparrow_get_did()->id);

  T.variable = VARIABLE_UNIT_BOOT_TIMER;
  T.element_size = 8;
  reply_len += sparrow_tlv_write_reply64int(&T, &reply[reply_len], len - reply_len, boottimerIEEE64());

  T.variable = VARIABLE_UNIT_CONTROLLER_WATCHDOG;
  T.element_size = 4;
  reply_len += sparrow_tlv_write_reply32int(&T, &reply[reply_len], len - reply_len, sparrow_uc_get_watchdog());

#ifdef VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX
  T.variable = VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX;
  T.element_size = 4;
  reply_len += sparrow_tlv_write_reply32int(&T, &reply[reply_len], len - reply_len,
                                            sparrow_oam_get_time_since_last_good_rx_from_uc());
#endif /* VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX */

  return reply_len;
}
/*---------------------------------------------------------------------------*/
static uint8_t
oam_notification(sparrow_oam_nt_t type, uint8_t operation)
{
  if(type == SPARROW_OAM_NT_NEW_PDU) {
    factory_default_state = FACTORY_DEFAULT_STATE_0;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
init(const sparrow_oam_instance_t *instance)
{
  instance->data->event_array[1] = 1;
  instance->data->sub_instance_count = sparrow_oam_get_instance_count() - 1;
}
/*---------------------------------------------------------------------------*/
static const sparrow_id_t *
get_i0_did(const sparrow_oam_instance_t *instance)
{
  return sparrow_get_did();
}
/*---------------------------------------------------------------------------*/
SPARROW_OAM_INSTANCE(instance0,
                     PRODUCT_TYPE_INT64, PRODUCT_LABEL,
                     instance0_variables,
                     .init = init,
                     .get_object_id = get_i0_did,
                     .process_request = process0_request,
                     .poll = poll0,
                     .notification = oam_notification);
/*---------------------------------------------------------------------------*/
