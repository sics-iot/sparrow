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
 *         Helper functions for portal selection
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 */

#include "contiki-conf.h"
#include "sparrow-psp.h"
#include "sparrow-uc.h"

#ifndef PRODUCT_TYPE_INT64
#error Please specify PRODUCT_TYPE_INT64 in project-conf.h
#endif /* PRODUCT_TYPE_INT64 */

/*
 * Set callback callback for portal selection status
 */
static void (*portal_status_cb)(uint32_t) = NULL;
/*---------------------------------------------------------------------------*/
/*
 * set portal selection status callback function to use.
 */
void
sparrow_psp_set_status_callback(void (*func)(uint32_t))
{
  portal_status_cb = func;
}
/*---------------------------------------------------------------------------*/
/*
 * Process Portal Selection Protocol response.
 * Do not reply on error, just silently drop.
 * Do not reply on success.
 */
size_t
sparrow_psp_process_request(const sparrow_tlv_t *request, uint8_t *reply, size_t len, sparrow_oam_processing_t *oam_processing)
{
  if((request->opcode == SPARROW_TLV_OPCODE_SET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST)) {
    /* write requests */
#ifdef VARIABLE_UNIT_CONTROLLER_ADDRESS
    uint8_t error;
    if(request->variable == VARIABLE_UNIT_CONTROLLER_ADDRESS) {
      error = sparrow_uc_set_address(request->data, TRUE);
      if(error == SPARROW_TLV_ERROR_NO_ERROR) {
        return sparrow_tlv_write_reply256(request, reply, len, 0);
      } else {
        return 0;
      }
    }
#endif /* #ifdef VARIABLE_UNIT_CONTROLLER_ADDRESS */
  } else if(request->opcode == SPARROW_TLV_OPCODE_GET_RESPONSE) {
#ifdef VARIABLE_PSP_PORTAL_STATUS
    if(request->variable == VARIABLE_PSP_PORTAL_STATUS) {
      if(portal_status_cb) {
        portal_status_cb(sparrow_tlv_get_int32_from_data(request->data));
      }
      return 0;
    }
#endif /* VARIABLE_PSP_PORTAL_STATUS */
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
#ifdef VARIABLE_CONFIGURATION_STATUS
/*
 * Get configuration status.
 */
static uint32_t
get_configuration_status(void)
{
  return 1;
}
#endif /* VARIABLE_CONFIGURATION_STATUS */
/*---------------------------------------------------------------------------*/
size_t
sparrow_psp_generate_request(uint8_t *reply, size_t len)
{
  int reply_len = 0;
  sparrow_tlv_t T;

  memset(&T, 0, sizeof(T));
  T.version = SPARROW_TLV_VERSION;
  T.opcode = SPARROW_TLV_OPCODE_GET_RESPONSE;
  T.error = SPARROW_TLV_ERROR_NO_ERROR;

  T.variable = VARIABLE_OBJECT_TYPE;
  T.element_size = 8;
  reply_len += sparrow_tlv_write_reply64int(&T, &reply[reply_len], len - reply_len, PRODUCT_TYPE_INT64);

  T.variable = VARIABLE_OBJECT_ID;
  T.element_size = 8;
  reply_len += sparrow_tlv_write_reply128(&T, &reply[reply_len], len - reply_len, sparrow_get_did()->id);

  memset(&T, 0, sizeof(T));
  T.version = SPARROW_TLV_VERSION;
  T.opcode = SPARROW_TLV_OPCODE_GET_RESPONSE;
  T.error = SPARROW_TLV_ERROR_NO_ERROR;

#ifdef VARIABLE_SW_REVISION
  T.variable = VARIABLE_SW_REVISION;
  T.element_size = 16;
  reply_len += sparrow_tlv_write_reply128(&T, &reply[reply_len], len - reply_len, sparrow_getswrevision());
#endif /* VARIABLE_SW_REVISION */

#ifdef VARIABLE_CONFIGURATION_STATUS
  T.variable = VARIABLE_CONFIGURATION_STATUS;
  T.element_size = 4;
  reply_len += sparrow_tlv_write_reply32int(&T, &reply[reply_len], len - reply_len,
                                 get_configuration_status());
#endif /* VARIABLE_CONFIGURATION_STATUS */

#ifdef VARIABLE_COMMUNICATION_STYLE
  T.variable = VARIABLE_COMMUNICATION_STYLE;
  T.element_size = 4;
  reply_len += sparrow_tlv_write_reply32int(&T, &reply[reply_len], len - reply_len,
                                           sparrow_uc_get_communication_style());
#endif /* VARIABLE_COMMUNICATION_STYLE */
  return reply_len;
}
/*---------------------------------------------------------------------------*/
