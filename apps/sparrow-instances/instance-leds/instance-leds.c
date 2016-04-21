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
 *         LEDs control instance.
 * \author
 *         Niclas Finne <nfi@sics.se>
 */

/**
 *
 * Leds, implements object 0x0090DA030201001E.
 *
 */

#include "contiki.h"
#include "dev/leds.h"
#include "dev/leds-ext.h"
#include "sparrow-oam.h"
#include "instance-leds-var.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
#ifdef INSTANCE_LEDS_CONF_NUMBER_OF_LEDS
#define LEDS_COUNT() (INSTANCE_LEDS_CONF_NUMBER_OF_LEDS)
#else
/* Count the number of set bits in LEDS_ALL if no configuration is set */
static int
get_number_of_leds(void)
{
  int c, count;

  for(count = 0, c = 0; c < 8; c++) {
    if(((1 << c) & LEDS_ALL) != 0) {
      count++;
    }
  }

  return count;
}
#define LEDS_COUNT() get_number_of_leds()
#endif /* INSTANCE_LEDS_NUMBER_OF_LEDS */
/*---------------------------------------------------------------------------*/
static uint8_t
get_leds_mask(uint8_t leds_info)
{
  int c, l;
  uint8_t mask = 0;
  for(c = 0, l = 0; c < 8; c++, l++) {
    /* Search for next used led */
    while(((LEDS_ALL & (1 << l)) == 0) && l < 8) {
      l++;
    }
    if(l == 8) {
      /* There are no more leds */
      break;
    }
    if(leds_info & (1 << c)) {
      mask |= (1 << l);
    }
  }
  return mask;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_leds_info(uint8_t leds_mask)
{
  int c, l;
  uint8_t info = 0;
  for(c = 0, l = 0; c < 8; c++) {
    if((LEDS_ALL & (1 << c)) == 0) {
      /* Led is not available */
      continue;
    }
    if(leds_mask & (1 << c)) {
      info |= (1 << l);
    }
    l++;
  }
  return info;
}
/*---------------------------------------------------------------------------*/
#if PLATFORM_HAS_LEDS_EXT
#ifdef VARIABLE_LED_STATE
static size_t
read_leds_state(sparrow_tlv_t *request, uint8_t *reply, size_t len)
{
  uint8_t buf[16 * 4];
  int i;
  if(request->offset >= leds_count ||
     request->elements == 0) {
    /* Nothing requested */
    return sparrow_tlv_write_reply_vector(request, reply, len, NULL);
  }
  if(request->elements > 16) {
    request->elements = 16;
  }
  if(request->offset + request->elements > leds_count) {
    request->elements = leds_count - request->offset;
  }
  for(i = 0; i < request->elements; i++) {
    sparrow_tlv_write_int32_to_buf(&buf[i * 4],
                                   leds_get_state(leds[request->offset + i]));
  }
  return sparrow_tlv_write_reply_vector(request, reply, len, buf);
}
#endif /* VARIABLE_LED_STATE */
#endif /* PLATFORM_HAS_LEDS_EXT */
/*---------------------------------------------------------------------------*/
#if PLATFORM_HAS_LEDS_EXT
#ifdef VARIABLE_LED_STATE
static size_t
write_leds_state(sparrow_tlv_t *request, uint8_t *reply, size_t len)
{
  int i;
  if(request->elements == 0) {
    /* Nothing requested */
    return sparrow_tlv_write_reply_vector(request, reply, len, NULL);
  }
  if(request->offset + request->elements > leds_count) {
    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_BAD_NUMBER_OF_ELEMENTS, reply, len);
  }
  for(i = 0; i < request->elements; i++) {
    leds_set_state(leds[request->offset + i],
                   (unsigned)sparrow_tlv_get_int32_from_data(&request->data[i * 4]));
  }
  return sparrow_tlv_write_reply_vector(request, reply, len, NULL);
}
#endif /* VARIABLE_LED_STATE */
#endif /* PLATFORM_HAS_LEDS_EXT */
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
  if(request->opcode == SPARROW_TLV_OPCODE_SET_REQUEST) {

    if(request->variable == VARIABLE_LED_CONTROL) {
      leds_set(get_leds_mask(request->data[3]));
      return sparrow_tlv_write_reply32(request, reply, len, NULL);
    }

#ifdef VARIABLE_LED_SET
    if(request->variable == VARIABLE_LED_SET) {
      leds_on(get_leds_mask(request->data[3]));
      return sparrow_tlv_write_reply32(request, reply, len, NULL);
    }
#endif /* VARIABLE_LED_SET */

#ifdef VARIABLE_LED_CLEAR
    if(request->variable == VARIABLE_LED_CLEAR) {
      leds_off(get_leds_mask(request->data[3]));
      return sparrow_tlv_write_reply32(request, reply, len, NULL);
    }
#endif /* VARIABLE_LED_CLEAR */

#ifdef VARIABLE_LED_TOGGLE
    if(request->variable == VARIABLE_LED_TOGGLE) {
      leds_toggle(get_leds_mask(request->data[3]));
      return sparrow_tlv_write_reply32(request, reply, len, NULL);
    }
#endif /* VARIABLE_LED_TOGGLE */

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED, reply, len);

  } else if(request->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST) {

#if PLATFORM_HAS_LEDS_EXT && defined(VARIABLE_LED_STATE)
    if(request->variable == VARIABLE_LED_STATE) {
      return write_leds_state(request, reply, len);
    }
#endif /* PLATFORM_HAS_LEDS_EXT && defined(VARIABLE_LED_STATE) */

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED, reply, len);

  } else if(request->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) {

    if(request->variable == VARIABLE_NUMBER_OF_LEDS) {
      return sparrow_tlv_write_reply32int(request, reply, len, LEDS_COUNT());

    } else if(request->variable == VARIABLE_LED_CONTROL) {
      return sparrow_tlv_write_reply32int(request, reply, len, get_leds_info(leds_get()));
    }

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

  } else if(request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST) {

#if PLATFORM_HAS_LEDS_EXT && defined(VARIABLE_LED_STATE)
    if(request->variable == VARIABLE_LED_STATE) {
      return read_leds_state(request, reply, len);
    }
#endif /* PLATFORM_HAS_LEDS_EXT && defined(VARIABLE_LED_STATE) */

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

  } else {
    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
  }
}
/*---------------------------------------------------------------------------*/
SPARROW_OAM_INSTANCE(instance_leds,
                     INSTANCE_LEDS_OBJECT_TYPE, INSTANCE_LEDS_LABEL,
                     instance_leds_variables,
                     .process_request = process_request);
/*---------------------------------------------------------------------------*/
