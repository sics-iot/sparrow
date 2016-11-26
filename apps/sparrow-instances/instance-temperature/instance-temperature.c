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
 */

/**
 * \file
 *         Temperature instance, implements object 0x0090DA0302010019.
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 */

#include "contiki.h"
#include "sparrow-oam.h"
#include "instance-temperature-var.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifndef INSTANCE_TEMPERATURE_ARCH_READVAL
  #error Temperature reading function (INSTANCE_TEMPERATURE_ARCH_READVAL) is not defined for instance-temperature.
#else
  uint32_t INSTANCE_TEMPERATURE_ARCH_READVAL(void);
#endif

#ifndef INSTANCE_TEMPERATURE_ARCH_ERRORVAL
  #error Temperature error value (INSTANCE_TEMPERATURE_ARCH_ERRORVAL) is not defined for instance-temperature.
#else
  uint32_t INSTANCE_TEMPERATURE_ARCH_ERRORVAL(void);
#endif

/*---------------------------------------------------------------------------*/

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
process_request(const sparrow_oam_instance_t *instance,
                sparrow_tlv_t *request, uint8_t *reply, size_t len,
                sparrow_oam_processing_t *oam_processing)
{
#if defined(INSTANCE_TEMPERATURE_ARCH_READVAL) && defined(INSTANCE_TEMPERATURE_ARCH_ERRORVAL)
  uint8_t error = SPARROW_TLV_ERROR_NO_ERROR;
  uint32_t local_data;
  if((request->opcode == SPARROW_TLV_OPCODE_SET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST)) {
    error = SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED;
  } else if((request->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST)) {

    if(request->variable == VARIABLE_TEMPERATURE) {
      local_data = INSTANCE_TEMPERATURE_ARCH_READVAL();
      if(local_data != INSTANCE_TEMPERATURE_ARCH_ERRORVAL()) {
        return sparrow_tlv_write_reply32int(request, reply, len, local_data);
      } else {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
      }
    }

    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }
#endif
  return sparrow_tlv_write_reply_error(request, error, reply, len);
}
/*---------------------------------------------------------------------------*/
static void
init(const sparrow_oam_instance_t *instance)
{
  instance->data->event_array[1] = 1;

#ifdef INSTANCE_TEMPERATURE_ARCH_INIT
  INSTANCE_TEMPERATURE_ARCH_INIT();
#endif
}
/*---------------------------------------------------------------------------*/
SPARROW_OAM_INSTANCE(instance_temperature,
                     INSTANCE_TEMPERATURE_OBJECT_TYPE,
                     INSTANCE_TEMPERATURE_LABEL,
                     instance_temperature_variables,
                     .init = init,
                     .process_request = process_request);
/*---------------------------------------------------------------------------*/
