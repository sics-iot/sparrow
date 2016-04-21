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

#ifndef SPARROW_OAM_H_
#define SPARROW_OAM_H_

#include "contiki.h"
#include "net/ip/uip.h"
#include "sparrow.h"
#include "sparrow-tlv.h"
#include "sparrow-encap.h"
#include "sparrow-var.h"

#ifndef SPARROW_OAM_PORT
#define SPARROW_OAM_PORT 49111
#endif /* SPARROW_OAM_PORT */

#include <stdio.h>
#define DEBUG_PRINT_(...) printf(__VA_ARGS__)
#define DEBUG_PRINT_I(...) printf(__VA_ARGS__)
#define DEBUG_PRINT_II(...) printf(__VA_ARGS__)

typedef enum {
  SPARROW_OAM_EVENT_BACKOFF_TIMER_START,
  SPARROW_OAM_EVENT_BACKOFF_TIMER_STOP,
  SPARROW_OAM_EVENT_BACKOFF_TIMER_ARE_WE_THERE_YET,
  SPARROW_OAM_EVENT_BACKOFF_TIMER_IS_ACTIVE,
  SPARROW_OAM_EVENT_BACKOFF_TIMER_GET_NEXT_TX_TIME,
} sparrow_oam_event_backoff_timer_cmd_t;

typedef enum {
  SPARROW_OAM_EVENTSTATE_EVENT_DISARMED = 0,
  SPARROW_OAM_EVENTSTATE_EVENT_ARMED    = 1,
  SPARROW_OAM_EVENTSTATE_EVENT_TRIGGED  = 3,
} sparrow_oam_event_state_t;
#define SPARROW_OAM_EVENTSTATE_OFFSET0_ENABLE_MASK  1
#define SPARROW_OAM_EVENTSTATE_OFFSET0_TRIGGED_MASK 2

typedef enum {
  SPARROW_OAM_EVENTACTION_EVENT_ARM    = 1,
  SPARROW_OAM_EVENTACTION_EVENT_DISARM = 0,
  SPARROW_OAM_EVENTACTION_EVENT_REARM  = 2,
} sparrow_oam_event_action_t;

typedef enum {
  SPARROW_OAM_NT_NEW_PDU,
  SPARROW_OAM_NT_UC_WD_EXPIRE,
  SPARROW_OAM_NT_SLEEP_WAKE,
  /* REPLY_SENT is used to indicate that a request have been processed
     and a reply have been sent (if any was to be sent) */
  SPARROW_OAM_NT_REPLY_SENT,
  SPARROW_OAM_NT_END_PDU,
  SPARROW_OAM_NT_SET_ACTIVITY_LEVEL,
  SPARROW_OAM_NT_FACTORY_DEFAULT
} sparrow_oam_nt_t;

typedef enum {
  SPARROW_OAM_NT_SLEEP_WAKE_SLEEP,
  SPARROW_OAM_NT_SLEEP_WAKE_WAKE,
} sparrow_oam_nt_sleep_wake_t;

#define SPARROW_OAM_PROCESSING_NEW 0x00
#define SPARROW_OAM_PROCESSING_ABORT_TLV_STACK 0x01
#define SPARROW_OAM_PROCESSING_DO_NOT_REPLY 0x02
#define SPARROW_OAM_PROCESSING_ENCRYPT_KEY_1 0x04
#define SPARROW_OAM_PROCESSING_ENCRYPT_KEY_3 0x08

typedef uint8_t sparrow_oam_processing_t;

typedef enum {
  SPARROW_OAM_POLL_TYPE_POLL,
  SPARROW_OAM_POLL_TYPE_EVENT,
} sparrow_oam_poll_type_t;

typedef enum {
  SPARROW_OAM_WRITABILITY_RO,
  SPARROW_OAM_WRITABILITY_RW,
  SPARROW_OAM_WRITABILITY_WO,
} sparrow_oam_writabilty_t;

typedef enum {
  SPARROW_OAM_FORMAT_ARRAY,
  SPARROW_OAM_FORMAT_ENUM,
  SPARROW_OAM_FORMAT_INTEGER,
  SPARROW_OAM_FORMAT_STRING,
  SPARROW_OAM_FORMAT_BLOB,
  SPARROW_OAM_FORMAT_TIME,
} sparrow_oam_format_t;
#define SPARROW_OAM_HAVE_FORMAT_TIME

typedef struct {
  uint16_t number;
  uint8_t  element_size;
  uint8_t  writability;
  uint8_t  format;
  uint8_t  vector_depth;
} sparrow_oam_variable_t;

#define SPARROW_OAM_VECTOR_DEPTH_DONT_CHECK 0xff

typedef struct {
  /* 2 * 32bit, as byte array. keep it in network byte order. */
  uint8_t event_array[8];
  uint8_t process; /* unused, external, ... */
  uint8_t flags;   /* newPDU.. */
  uint8_t sub_instance_count;
  uint8_t instance_id;
} sparrow_oam_instance_data_t;

typedef struct sparrow_oam_instance sparrow_oam_instance_t;
struct sparrow_oam_instance {
  sparrow_oam_instance_data_t *data;
  const sparrow_oam_variable_t *variables;
  const uint16_t variable_count;
  const char *label;
  uint64_t object_type;

  void (* init)(const sparrow_oam_instance_t *instance);
  const sparrow_id_t *(* get_object_id)(const sparrow_oam_instance_t *instance);
  uint32_t (* get_object_sub_type)(const sparrow_oam_instance_t *instance);
  size_t (*process_request)(const sparrow_oam_instance_t *instance, sparrow_tlv_t *request, uint8_t *reply, size_t len, sparrow_oam_processing_t *oam_processing);
  size_t (*poll)(const sparrow_oam_instance_t *instance, uint8_t *reply, size_t len, sparrow_oam_poll_type_t type);
  uint8_t (*notification)(sparrow_oam_nt_t o, uint8_t operation);
};

#define SPARROW_OAM_INSTANCE(name, object_type, label, variables, ...)  \
  static sparrow_oam_instance_data_t name##_data_;                      \
  const sparrow_oam_instance_t name = {                                 \
    &name##_data_,                                                      \
    variables, sizeof(variables) / sizeof(sparrow_oam_variable_t),      \
    label, object_type,                                                 \
    __VA_ARGS__                                                         \
  }

#define SPARROW_OAM_INSTANCE_NAME(name)                                 \
  extern const sparrow_oam_instance_t name

typedef struct sparrow_oam_notification_handle sparrow_oam_notification_handle_t;
struct sparrow_oam_notification_handle {
  sparrow_oam_notification_handle_t *next;
  uint8_t (*oam_notification)(sparrow_oam_nt_t o, uint8_t operation);
};

#define SPARROW_OAM_INSTANCE_PROCESS_NORMAL        0
#define SPARROW_OAM_INSTANCE_PROCESS_EXTERNAL      1
#define SPARROW_OAM_INSTANCE_PROCESS_SILENT_IGNORE 2

typedef enum {
  SPARROW_OAM_ACTIVITY_TYPE_ANY,
  SPARROW_OAM_ACTIVITY_TYPE_TX,
  SPARROW_OAM_ACTIVITY_TYPE_RX,
  SPARROW_OAM_ACTIVITY_TYPE_RX_FROM_UC,
} sparrow_oam_activity_type_t;

void sparrow_oam_init(void);
const sparrow_oam_instance_t *sparrow_oam_get_instance(uint8_t instance_id);
const sparrow_oam_instance_t **sparrow_oam_get_instances(void);
uint8_t sparrow_oam_get_instance_count(void);
uint8_t sparrow_oam_get_instance_id(const sparrow_oam_instance_t *instance);

/*
 * tools
 */

uint64_t sparrow_oam_event_backoff_timer(sparrow_oam_event_backoff_timer_cmd_t cmd);
size_t sparrow_oam_generate_event_reply(uint8_t *reply, size_t len);
void sparrow_oam_pdu_begin(void);
void sparrow_oam_pdu_end(void);

uint8_t sparrow_oam_do_event_action(uint8_t *state, uint8_t action);

void sparrow_oam_send_udp_packet(const uip_ipaddr_t *destination, uint16_t port, const uint8_t *data, int len);

uint8_t sparrow_oam_echo_replies(uint8_t increment);

uint8_t sparrow_oam_is_event_triggered(const sparrow_oam_instance_t *instance);
void sparrow_oam_trig_event(const sparrow_oam_instance_t *instance);

void sparrow_oam_tx_activity(void);
uint64_t sparrow_oam_last_activity(sparrow_oam_activity_type_t type);
uint32_t sparrow_oam_get_time_since_last_good_rx_from_uc(void);

void sparrow_oam_schedule_wakeup_notification(void);

void sparrow_oam_notify_all_instances(sparrow_oam_nt_t type, uint8_t operation);

void sparrow_oam_notify_all_sub_instances(sparrow_oam_nt_t type, uint8_t operation);

size_t sparrow_oam_process_request(const uint8_t *request, size_t len,
                                   uint8_t *reply, size_t replymax,
                                   sparrow_encap_pdu_info_t *pinfo);
size_t sparrow_oam_process_discovery_request(const sparrow_tlv_t *request, uint8_t *reply, size_t len, sparrow_oam_processing_t *oam_processing);

#endif /* SPARROW_OAM_H_ */
