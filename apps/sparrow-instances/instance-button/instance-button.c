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
 *         Button instance, implements object 0x0090DA030201001D.
 * \author
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"
#include "dev/button-sensor.h"
#include "sparrow-oam.h"
#include "instance-button.h"
#include "instance-button-var.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

PROCESS(instance_button_process, "Button instance");

SPARROW_OAM_INSTANCE_NAME(instance_button);

static uint32_t trigger_counter = 0;
static uint8_t trigger_type[4] = { 0, };
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
 * Returns number of bytes written to "reply".
 */
static size_t
process_request(const sparrow_oam_instance_t *instance,
                sparrow_tlv_t *request, uint8_t *reply, size_t len,
                sparrow_oam_processing_t *oam_processing)
{
  uint32_t data32;

  if(request->opcode == SPARROW_TLV_OPCODE_SET_REQUEST) {

    if(request->variable == VARIABLE_GPIO_TRIGGER_TYPE) {
      /*
       * Trigger type
       */
      trigger_type[0] = request->data[0];
      trigger_type[1] = request->data[1];
      trigger_type[2] = request->data[2];
      trigger_type[3] = request->data[3];
      return sparrow_tlv_write_reply32(request, reply, len, NULL);
    }

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED, reply, len);

  } else if(request->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) {

    if(request->variable == VARIABLE_GPIO_INPUT) {
      /*
       * GPIO input
       */
      data32 = button_sensor.value(0);
      return sparrow_tlv_write_reply32int(request, reply, len, data32);
    } else if(request->variable == VARIABLE_GPIO_TRIGGER_COUNTER) {
      /*
       * Trigger counter
       */
      data32 = trigger_counter;
      return sparrow_tlv_write_reply32int(request, reply, len, data32);
    } else if(request->variable == VARIABLE_GPIO_TRIGGER_TYPE) {
      /*
       * Trigger type
       */
      return sparrow_tlv_write_reply32(request, reply, len, trigger_type);
    }

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

  } else {
    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
  }
}
/*---------------------------------------------------------------------------*/

/**
 * Process a poll request.
 * Write a reply TLV to "reply", no more than "len" bytes.
 * Return number of bytes written to "reply".
 */
static size_t
do_poll(const sparrow_oam_instance_t *instance, uint8_t *reply, size_t len,
        sparrow_oam_poll_type_t poll_type)
{
  int reply_len = 0;
  sparrow_tlv_t T;

  if(poll_type == SPARROW_OAM_POLL_TYPE_EVENT) {
    sparrow_tlv_init_get32(&T, sparrow_oam_get_instance_id(instance),
                           VARIABLE_GPIO_TRIGGER_COUNTER);
    reply_len = sparrow_tlv_write_reply32int(&T, reply, len, trigger_counter);
  }

  return reply_len;
}
/*---------------------------------------------------------------------------*/
uint32_t
instance_button_get(void)
{
  return button_sensor.value(0);
}
/*---------------------------------------------------------------------------*/
uint32_t
instance_button_get_counter(void)
{
  return trigger_counter;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(instance_button_process, ev, data)
{
  PROCESS_BEGIN();

  SENSORS_ACTIVATE(button_sensor);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    trigger_counter++;
    PRINTF("button: pressed %lu times\n", trigger_counter);

    /* If event is not armed, it did not happen */
    if(instance_button.data->event_array[7] == SPARROW_OAM_EVENTSTATE_EVENT_ARMED) {
      sparrow_oam_trig_event(&instance_button);
    } else {
      PRINTF("button: event not armed (%d)\n", instance_button.data->event_array[7]);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
init(const sparrow_oam_instance_t *instance)
{
  instance->data->event_array[1] = 1;
  process_start(&instance_button_process, NULL);
}
/*---------------------------------------------------------------------------*/
SPARROW_OAM_INSTANCE(instance_button,
                     INSTANCE_BUTTON_OBJECT_TYPE, INSTANCE_BUTTON_LABEL,
                     instance_button_variables,
                     .init = init,
                     .process_request = process_request,
                     .poll = do_poll);
/*---------------------------------------------------------------------------*/
