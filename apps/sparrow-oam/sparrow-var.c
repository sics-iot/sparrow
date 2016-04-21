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
 */

/**
 * \file
 *         Functions for handling common variables in Sparrow instances.
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 */

#include "sparrow-oam.h"
/*---------------------------------------------------------------------------*/
/* Default discovery variables */
static const sparrow_oam_variable_t discovery_variables[] = {
  { 0x000,  8, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 }, /* VARIABLE_OBJECT_TYPE */
  { 0x001, 16, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 }, /* VARIABLE_OBJECT_ID */
  { 0x002, 32, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_STRING ,  0 }, /* VARIABLE_OBJECT_LABEL */
  { 0x003,  4, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 }, /* VARIABLE_NUMBER_OF_INSTANCES */
  { 0x004,  4, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 }, /* VARIABLE_OBJECT_SUB_TYPE */
  { 0x005,  4, SPARROW_OAM_WRITABILITY_RW, SPARROW_OAM_FORMAT_INTEGER,  2 }, /* VARIABLE_EVENT_ARRAY */
};
#define DISCOVERY_COUNT (sizeof(discovery_variables) / sizeof(sparrow_oam_variable_t))
/*---------------------------------------------------------------------------*/
/*
 * Verify that this TLV is correct and allowed for this variable.
 */
sparrow_tlv_error_t
sparrow_var_check_tlv_variable(const sparrow_tlv_t *t,
                               const sparrow_oam_variable_t *v)
{
  int expected_length;
  if(!v) {
    return SPARROW_TLV_ERROR_UNKNOWN_VARIABLE;
  }

  // Vector request checks
  if(t->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    /* No offset or length verification necessary on blobs */
    if(t->opcode != SPARROW_TLV_OPCODE_VECTOR_BLOB_REQUEST) {
      if(v->vector_depth < 1) {
        DEBUG_PRINT_("not-blob vector is no vector\n");
        sparrow_tlv_print(t);
        return SPARROW_TLV_ERROR_NO_VECTOR_ACCESS;
      }
      if(v->vector_depth != SPARROW_OAM_VECTOR_DEPTH_DONT_CHECK) {
        if(t->offset > (v->vector_depth -1)) {
          return SPARROW_TLV_ERROR_INVALID_VECTOR_OFFSET;
        }
      }
    }
  } else {
    /* Blob MUST be vector */
    if(t->opcode == SPARROW_TLV_OPCODE_BLOB_REQUEST) {
      DEBUG_PRINT_("Blob is no vector\n");
      return SPARROW_TLV_ERROR_NO_VECTOR_ACCESS;
    }
  }

  /*
   * Verify element size. All variables with format TIME is 64bit, enforced
   * by the variable checker, also allow it to be accessed as 32bit.
   */
  if(v->element_size != t->element_size) {
#ifndef HAVE_FORMAT_TIME
    return SPARROW_TLV_ERROR_UNKNOWN_ELEMENT_SIZE;
#else  /* HAVE_FORMAT_TIME */
    if(v->format != FORMAT_TIME) {
      return SPARROW_TLV_ERROR_UNKNOWN_ELEMENT_SIZE;
    }
    if(v->element_size != 4) {
      return SPARROW_TLV_ERROR_UNKNOWN_ELEMENT_SIZE;
    }
#endif /* HAVE_FORMAT_TIME */
  }

  /* Verify element size */
  if(v->element_size != t->element_size) {
    return SPARROW_TLV_ERROR_UNKNOWN_ELEMENT_SIZE;
  }

  /* Verify length */
  if(t->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    expected_length = 16;
  } else {
    expected_length = 8;
  }
  if(sparrow_tlv_with_data(t->opcode)) {
    if(t->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
      expected_length += (t->element_size * t->elements);
    } else {
      expected_length += t->element_size;
    }
  }

  if(expected_length != t->length) {
    return SPARROW_TLV_ERROR_BAD_LENGTH;
  }

  if(v->writability == SPARROW_OAM_WRITABILITY_RO) {
    /* write access */
    if((t->opcode == SPARROW_TLV_OPCODE_SET_REQUEST) ||
       (t->opcode == SPARROW_TLV_OPCODE_MASKED_SET_REQUEST) ||
       (t->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST) ||
       (t->opcode == SPARROW_TLV_OPCODE_VECTOR_MASKED_SET_REQUEST)) {
      return SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED;
    }
  } else if(v->writability == SPARROW_OAM_WRITABILITY_WO) {
    /* read access */
    if((t->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) ||
       (t->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST)) {
      return SPARROW_TLV_ERROR_READ_ACCESS_DENIED;
    }
  }

  return SPARROW_TLV_ERROR_NO_ERROR;
}
/*---------------------------------------------------------------------------*/
/*
 * Verify that this TLV is correct and allowed.
 */
sparrow_tlv_error_t
sparrow_var_check_tlv(const sparrow_tlv_t *t, uint8_t is_psp)
{
  const sparrow_oam_instance_t *instance;
  int i;

  /* check version */
  if(t->version != SPARROW_TLV_VERSION) {
    return SPARROW_TLV_ERROR_UNKNOWN_VERSION;
  }

  /* Check instance */
  if(t->instance >= sparrow_oam_get_instance_count()) {
    return SPARROW_TLV_ERROR_UNKNOWN_INSTANCE;
  }

  /* Verify that is is a request, if it isn't a PSP */
  if(!is_psp) {
    if(t->opcode & SPARROW_TLV_OPCODE_REPLY_MASK) {
      return SPARROW_TLV_ERROR_UNPROCESSED_TLV; /* silently skip responses */
    }
  }

  if(sparrow_tlv_is_discovery_variable(t)) {
    for(i = 0; i < DISCOVERY_COUNT; i++) {
      if(discovery_variables[i].number == t->variable) {
        return sparrow_var_check_tlv_variable(t, &discovery_variables[i]);
      }
    }
  }

  instance = sparrow_oam_get_instances()[t->instance];
  for(i = 0; i < instance->variable_count; i++) {
    if(instance->variables[i].number == t->variable) {
      return sparrow_var_check_tlv_variable(t, &instance->variables[i]);
    }
  }

  return SPARROW_TLV_ERROR_UNKNOWN_VARIABLE;
}
/*---------------------------------------------------------------------------*/
/*
 * Called whenever an event is acknowledged.
 */
void
sparrow_var_update_event_arrays(void)
{
  const sparrow_oam_instance_t **instances;
  int i;
  int instance;
  int offset;
  int instance_trigged;
  int unit_trigged = 0;
  uint8_t vector_depth;

  instances = sparrow_oam_get_instances();

  for(instance = 1; instances[instance] != NULL; instance++) {
    vector_depth = SPARROW_OAM_VECTOR_DEPTH_DONT_CHECK;
    instance_trigged = 0;
    for(i = 0; i < instances[instance]->variable_count; i++) {
      if(instances[instance]->variables[i].number == VARIABLE_EVENT_ARRAY) {
        vector_depth = instances[instance]->variables[i].vector_depth;
        break;
      }
    }
    if(vector_depth == SPARROW_OAM_VECTOR_DEPTH_DONT_CHECK) {
      continue;
    }
    for(offset = 1; offset < vector_depth; offset++) {
      if(instances[instance]->data->event_array[(offset * 4) + 3] == SPARROW_OAM_EVENTSTATE_EVENT_TRIGGED) {
        instance_trigged++;
      }
    }
    if(instance_trigged > 0) {
      instances[instance]->data->event_array[3] |= SPARROW_OAM_EVENTSTATE_OFFSET0_TRIGGED_MASK;
    } else {
      instances[instance]->data->event_array[3] &= SPARROW_OAM_EVENTSTATE_OFFSET0_ENABLE_MASK;
    }
    unit_trigged += instance_trigged;
  }
  if(unit_trigged > 0) {
    instances[0]->data->event_array[3] |= SPARROW_OAM_EVENTSTATE_OFFSET0_TRIGGED_MASK;
  } else {
    instances[0]->data->event_array[3] &= SPARROW_OAM_EVENTSTATE_OFFSET0_ENABLE_MASK;
    sparrow_oam_event_backoff_timer(SPARROW_OAM_EVENT_BACKOFF_TIMER_STOP);
  }
}
/*---------------------------------------------------------------------------*/
