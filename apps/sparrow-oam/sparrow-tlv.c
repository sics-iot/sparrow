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
 *         Sparrow TLV format implemention
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 */

#include "sparrow-oam.h"
#include "sparrow-tlv.h"
#include <string.h>
#include <inttypes.h>

const uint8_t sparrow_tlv_zeroes[16] = { 0, };
/*---------------------------------------------------------------------------*/
void
sparrow_tlv_init_tlv(sparrow_tlv_t *t, uint8_t instance, uint16_t variable)
{
  memset(t, 0, sizeof(sparrow_tlv_t));
  t->version = SPARROW_TLV_VERSION;
  t->instance = instance;
  t->error = SPARROW_TLV_ERROR_NO_ERROR;
  t->variable = variable;
}
/*---------------------------------------------------------------------------*/
void
sparrow_tlv_init_get32(sparrow_tlv_t *t, uint8_t instance, uint16_t variable)
{
  sparrow_tlv_init_tlv(t, instance, variable);
  t->opcode = SPARROW_TLV_OPCODE_GET_REQUEST;
  t->element_size = 4;
}
/*---------------------------------------------------------------------------*/
void
sparrow_tlv_init_get64(sparrow_tlv_t *t, uint8_t instance, uint16_t variable)
{
  sparrow_tlv_init_tlv(t, instance, variable);
  t->opcode = SPARROW_TLV_OPCODE_GET_REQUEST;
  t->element_size = 8;
}
/*---------------------------------------------------------------------------*/
/*
 * Check if this TLV contains a discovery variable request or
 * response.
 */
uint8_t
sparrow_tlv_is_discovery_variable(const sparrow_tlv_t *t)
{
  if(t->variable < 0x20) {
    return TRUE;
  }
  return FALSE;
}
/*---------------------------------------------------------------------------*/
/*
 * Set reply bit on opcode in TLV pointed to by "t".
 */
void
sparrow_tlv_replyify(sparrow_tlv_t *t)
{
  t->opcode = t->opcode | SPARROW_TLV_OPCODE_REPLY_MASK;
}
/*---------------------------------------------------------------------------*/
void
sparrow_tlv_print(const sparrow_tlv_t *t)
{
  DEBUG_PRINT_I("oam: Version     = %d\n", t->version);
  DEBUG_PRINT_I("oam: Length      = %u\n", t->length);
  DEBUG_PRINT_I("oam: Variable    = 0x%03x\n", t->variable);
  DEBUG_PRINT_I("oam: Instance    = %d\n", t->instance);
  DEBUG_PRINT_I("oam: Opcode      = %d\n", t->opcode);
  DEBUG_PRINT_I("oam: ElementSize = %d\n", t->element_size);
  DEBUG_PRINT_I("oam: Error       = %d\n", t->error);
  if(t->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    DEBUG_PRINT_I("oam: Offset      = %"PRIu32"\n", t->offset);
    DEBUG_PRINT_I("oam: Elements    = %"PRIu32"\n", t->elements);
  }
}
/*---------------------------------------------------------------------------*/
/*
 * Return length of TLV pointed to by "p", in number of bytes.
 */
size_t
sparrow_tlv_length_from_data(const uint8_t *p)
{
  return (((p[0] & 0xf) << 8) + p[1]) * 4;
}
/*---------------------------------------------------------------------------*/
/*
 * Return true if this opcode is carrying data.
 */
uint8_t
sparrow_tlv_with_data(sparrow_tlv_opcode_t o)
{
  uint8_t retval;
  switch(o) {
  case SPARROW_TLV_OPCODE_GET_RESPONSE:
  case SPARROW_TLV_OPCODE_SET_REQUEST:
  case SPARROW_TLV_OPCODE_REFLECT_REQUEST:
  case SPARROW_TLV_OPCODE_REFLECT_RESPONSE:
  case SPARROW_TLV_OPCODE_MASKED_SET_REQUEST:
  case SPARROW_TLV_OPCODE_ABORT_IF_EQUAL_REQUEST:
  case SPARROW_TLV_OPCODE_BLOB_REQUEST:
  case SPARROW_TLV_OPCODE_BLOB_RESPONSE:
  case SPARROW_TLV_OPCODE_VECTOR_GET_RESPONSE:
  case SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST:
  case SPARROW_TLV_OPCODE_VECTOR_REFLECT_REQUEST:
  case SPARROW_TLV_OPCODE_VECTOR_REFLECT_RESPONSE:
  case SPARROW_TLV_OPCODE_VECTOR_EVENT_RESPONSE:
  case SPARROW_TLV_OPCODE_VECTOR_MASKED_SET_REQUEST:
  case SPARROW_TLV_OPCODE_VECTOR_ABORT_IF_EQUAL_REQUEST:
  case SPARROW_TLV_OPCODE_VECTOR_BLOB_REQUEST:
  case SPARROW_TLV_OPCODE_VECTOR_BLOB_RESPONSE:
    retval = TRUE;
    break;

  case SPARROW_TLV_OPCODE_GET_REQUEST:
  case SPARROW_TLV_OPCODE_SET_RESPONSE:
  case SPARROW_TLV_OPCODE_EVENT_REQUEST:
  case SPARROW_TLV_OPCODE_EVENT_RESPONSE:
  case SPARROW_TLV_OPCODE_MASKED_SET_RESPONSE:
  case SPARROW_TLV_OPCODE_ABORT_IF_EQUAL_RESPONSE:
  case SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST:
  case SPARROW_TLV_OPCODE_VECTOR_SET_RESPONSE:
  case SPARROW_TLV_OPCODE_VECTOR_EVENT_REQUEST:
  case SPARROW_TLV_OPCODE_VECTOR_MASKED_SET_RESPONSE:
  case SPARROW_TLV_OPCODE_VECTOR_ABORT_IF_EQUAL_RESPONSE:
    retval = FALSE;
    break;

  default:
    retval = FALSE;
    break;
  }
  return retval;
}
/*---------------------------------------------------------------------------*/
/*
 * Combine 2 bytes of data from an array to a host byte order int 16.
 */
uint16_t
sparrow_tlv_get_int16_from_data(const uint8_t *p)
{
  return (p[0] << 8) + p[1];
}
/*---------------------------------------------------------------------------*/
/*
 * Write 16 bit host byte order value to byte array in network byte order.
 */
void
sparrow_tlv_write_int16_to_buf(uint8_t *p, uint16_t d)
{
  p[0] = (d >> 8) & 0xff;
  p[1] = d & 0xff;
}
/*---------------------------------------------------------------------------*/
/*
 * Combine 4 bytes of data from an array to a host byte order int 32.
 */
uint32_t
sparrow_tlv_get_int32_from_data(const uint8_t *p)
{
  return ((uint32_t)p[0] << 24) + ((uint32_t)p[1] << 16) + (p[2] << 8) + p[3];
}
/*---------------------------------------------------------------------------*/
/*
 * Write 32 bit host byte order value to byte array in network byte order.
 */
void
sparrow_tlv_write_int32_to_buf(uint8_t *p, uint32_t d)
{
  p[0] = (d >> 24) & 0xff;
  p[1] = (d >> 16) & 0xff;
  p[2] = (d >> 8) & 0xff;
  p[3] = d & 0xff;
}
/*---------------------------------------------------------------------------*/
/*
 * Combine 8 bytes of data from an array to a host byte order int 64.
 */
uint64_t
sparrow_tlv_get_int64_from_data(const uint8_t *p)
{
  uint64_t retval = 0;
  int i;
  for(i = 0 ; i < 8; i++) {
    retval = retval << 8;
    retval |= p[i];
  }
  return retval;
}
/*---------------------------------------------------------------------------*/
/*
 * Write 64 bit host byte order value to byte array in network byte order.
 */
void
sparrow_tlv_write_int64_to_buf(uint8_t *p, uint64_t d)
{
  int i;
  for(i = 0; i < 8; i++) {
    p[i] = (d >> ((7 - i)*8)) & 0xff;
  }
}
/*---------------------------------------------------------------------------*/
/*
 * Parse a TLV in a byte array.
 * Note that length and elementsize fields are converted to indicate length in bytes.
 * Data (if any) is not copied, tlv->data is set to point to it.
 * Return number of bytes consumed by this TLV.
 */
size_t
sparrow_tlv_from_bytes(sparrow_tlv_t *t, const uint8_t *in_data)
{
  t->version = in_data[0] >> 4;
  t->length = (((in_data[0] & 0xf) << 8) + in_data[1]) * 4;
  t->variable = (in_data[2] << 8) + in_data[3];
  t->instance = in_data[4];
  t->opcode = in_data[5];
  t->element_size = 1 << (in_data[6] + 2);
  t->error = in_data[7];

  if(t->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    t->offset = sparrow_tlv_get_int32_from_data(&in_data[8]);
    t->elements = sparrow_tlv_get_int32_from_data(&in_data[12]);
    t->data = &in_data[16];
  } else {
    t->offset = 0;
    t->elements = 1;
    t->data = in_data + 8;
  }
  if(!sparrow_tlv_with_data(t->opcode)) {
    t->data = NULL;
  }
  return t->length;
}
/*---------------------------------------------------------------------------*/
/*
 * Write a TLV, including copy the data (if any), to a destination
 * buffer, no more than "len" bytes.
 *
 * Return number of bytes written.
 */
size_t
sparrow_tlv_to_bytes(const sparrow_tlv_t *t, uint8_t *data, size_t len)
{
  size_t retval = 8;
  size_t data_size = 0;
  uint8_t *data_start = NULL;
  uint8_t i;
  if(t->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    retval += 8;
  }
  if(sparrow_tlv_with_data(t->opcode)) {
    if(t->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
      data_size = t->elements * t->element_size;
      data_start = data + 16;
    } else {
      data_size = t->element_size;
      data_start = data + 8;
    }
  } else {
    data_size = 0;
    data_start = NULL;
  }
  retval += data_size;

  /* not enough space in the buffer */
  if(retval > len) {
    return 0;
  }

  /*
   * retval contains length in bytes so shifts are +2 to convert to
   * 32bit words.
   */
  data[0] = ((t->version & 0x0f) << 4) + ((retval >> 10) & 0xff);
  data[1] = (retval >> 2) & 0xff;
  data[2] = t->variable >> 8;
  data[3] = t->variable & 0xff;
  data[4] = t->instance;
  data[5] = t->opcode;
  for(i = 0; t->element_size > (1 << (i + 2)); i++);
  data[6] = i;
  data[7] = t->error;
  if(t->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    data[8] = (t->offset >> 24) & 0xff;
    data[9] = (t->offset >> 16) & 0xff;
    data[10] = (t->offset >> 8) & 0xff;
    data[11] = t->offset & 0xff;
    data[12] = (t->elements >> 24) & 0xff;
    data[13] = (t->elements >> 16) & 0xff;
    data[14] = (t->elements >> 8) & 0xff;
    data[15] = t->elements & 0xff;
  }

  /*
   * If there is data to copy and the source for data is zero, clear
   * destiantiom, else copy data if it's not already in place.
   */
  if((data_start != NULL) && (data_size > 0)) {
    if(t->data == 0) {
      memset(data_start, 0, data_size);
    } else if(data_start != t->data) {
      memcpy(data_start, t->data, data_size);
    }
  }
  return retval;
}
/*---------------------------------------------------------------------------*/
/*
 * Construct an error TLV with the given error, without data. All
 * other fields are taken from "request".
 * Error TLV is written to "reply", no more than "len" bytes.
 * Return number of bytes written.
 */
size_t
sparrow_tlv_write_reply_error(const sparrow_tlv_t *request, uint8_t error,
                              uint8_t *reply, size_t len)
{
  sparrow_tlv_t T;
  T.version = request->version;
  if(request->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    T.length = 16;
    T.elements = request->elements;
  } else {
    T.length = 8;
  }
  T.variable = request->variable;
  T.instance = request->instance;
  T.element_size = request->element_size;
  T.offset = request->offset;
  T.error = error;
  T.opcode = request->opcode;
  T.data = 0;
  sparrow_tlv_replyify(&T);
  return sparrow_tlv_to_bytes(&T, reply, len);
}
/*---------------------------------------------------------------------------*/
/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and
 * should already be in network byte order.
 */
size_t
sparrow_tlv_write_reply32(const sparrow_tlv_t *request,
                          uint8_t *reply, size_t len, const uint8_t *data)
{
  sparrow_tlv_t T;

  if(request->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    return 0;
  }
  T.version = request->version;
  T.length = 12;  /* single 32bit reply */
  T.variable = request->variable;
  T.instance = request->instance;
  T.element_size = 4;
  T.offset = request->offset;
  T.error = SPARROW_TLV_ERROR_NO_ERROR;
  T.opcode = request->opcode;
  sparrow_tlv_replyify(&T);
  T.data = data;
  return sparrow_tlv_to_bytes(&T, reply, len);
}
/*---------------------------------------------------------------------------*/
/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and is
 * converted to network byte order.
 */
size_t
sparrow_tlv_write_reply32int(const sparrow_tlv_t *request,
                             uint8_t *reply, size_t len, uint32_t data)
{
  uint8_t p[4];
  sparrow_tlv_write_int32_to_buf(p, data);
  return sparrow_tlv_write_reply32(request, reply, len, p);
}
/*---------------------------------------------------------------------------*/
/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and
 * should already be in network byte order.
 */
size_t
sparrow_tlv_write_reply64(const sparrow_tlv_t *request,
                          uint8_t *reply, size_t len, const uint8_t *data)
{
  sparrow_tlv_t T;

  if(request->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    return 0;
  }
  T.version = request->version;
  T.length = 16;   /* single 64bit reply */
  T.variable = request->variable;
  T.instance = request->instance;
  T.element_size = 8;
  T.offset = request->offset;
  T.error = SPARROW_TLV_ERROR_NO_ERROR;
  T.opcode = request->opcode;
  sparrow_tlv_replyify(&T);
  T.data = data;
  return sparrow_tlv_to_bytes(&T, reply, len);
}
/*---------------------------------------------------------------------------*/
/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes", with payload "data". Data is converted to
 * network byte order.
 */
size_t
sparrow_tlv_write_reply64int(const sparrow_tlv_t *request,
                             uint8_t *reply, size_t len, uint64_t data)
{
  uint8_t p[8];
  sparrow_tlv_write_int64_to_buf(p, data);
  return sparrow_tlv_write_reply64(request, reply, len, p);
}
/*---------------------------------------------------------------------------*/
/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and
 * should already be in network byte order.
 */
size_t
sparrow_tlv_write_reply128(const sparrow_tlv_t *request,
                           uint8_t *reply, size_t len, const uint8_t *data)
{
  sparrow_tlv_t T;

  if(request->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    return 0;
  }
  T.version = request->version;
  T.length = 24;   /* single 128bit reply */
  T.variable = request->variable;
  T.instance = request->instance;
  T.element_size = 16;
  T.offset = request->offset;
  T.error = SPARROW_TLV_ERROR_NO_ERROR;
  T.opcode = request->opcode;
  sparrow_tlv_replyify(&T);
  T.data = data;

  return sparrow_tlv_to_bytes(&T, reply, len);
}
/*---------------------------------------------------------------------------*/
/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and
 * should already be in network byte order.
 */
size_t
sparrow_tlv_write_reply256(const sparrow_tlv_t *request,
                           uint8_t *reply, size_t len, const uint8_t *data)
{
  sparrow_tlv_t T;

  if(request->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    return 0;
  }
  T.version = request->version;
  T.length = 40;  /* single 32byte reply */
  T.variable = request->variable;
  T.instance = request->instance;
  T.element_size = 32;
  T.offset = 0;
  T.error = SPARROW_TLV_ERROR_NO_ERROR;
  T.opcode = request->opcode;
  sparrow_tlv_replyify(&T);
  T.data = data;

  return sparrow_tlv_to_bytes(&T, reply, len);
}
/*---------------------------------------------------------------------------*/
/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and
 * should already be in network byte order.
 */
size_t
sparrow_tlv_write_reply512(const sparrow_tlv_t *request,
                           uint8_t *reply, size_t len, const uint8_t *data)
{
  sparrow_tlv_t T;

  if(request->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) {
    return 0;
  }
  T.version = request->version;
  T.length = 8 + 64;
  T.variable = request->variable;
  T.instance = request->instance;
  T.element_size = 64;
  T.offset = 0;
  T.error = SPARROW_TLV_ERROR_NO_ERROR;
  T.opcode = request->opcode;
  sparrow_tlv_replyify(&T);
  T.data = data;

  return sparrow_tlv_to_bytes(&T, reply, len);
}
/*---------------------------------------------------------------------------*/
/*
 * Write a vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data" with
 * offset and elements from request.  No byteorder conversion is done.
 */
size_t
sparrow_tlv_write_reply_vector(const sparrow_tlv_t *request,
                               uint8_t *reply, size_t len,
                               const uint8_t *data)
{
  sparrow_tlv_t T;

  if(!(request->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK)) {
    DEBUG_PRINT_("writeReplyVectorTlv: not a vector\n");
    return 0;
  }
  T.version = request->version;
  T.elements = request->elements;
  T.element_size = request->element_size;
  T.length = 16 + (T.elements * T.element_size);
  T.variable = request->variable;
  T.instance = request->instance;
  T.offset = request->offset;
  T.error = SPARROW_TLV_ERROR_NO_ERROR;
  T.opcode = request->opcode;
  sparrow_tlv_replyify(&T);
  if(data == NULL) {
    T.data = NULL;
  } else {
    T.data = data + (T.element_size * T.offset);
  }
  return sparrow_tlv_to_bytes(&T, reply, len);
}
/*---------------------------------------------------------------------------*/
/*
 * Write a non-vector Blob reply TLV based on "request" to "reply", no
 * more than "len" bytes".
 *
 * The payload data is read from "data", "dataLen" number of bytes,
 * and should already be in network byte order. Data should already be
 * padded with zero to n*4 bytes.
 */
size_t
sparrow_tlv_write_reply_blob(const sparrow_tlv_t *request,
                             uint8_t *reply, size_t len,
                             const uint8_t *data, size_t dataLen)
{
  sparrow_tlv_t T;
  if((request->opcode & SPARROW_TLV_OPCODE_VECTOR_MASK) == 0) {
    return 0;
  }
  T.version = request->version;
  T.length = 16 + dataLen;
  T.variable = request->variable;
  T.instance = request->instance;
  T.element_size = 4;
  T.offset = 0;
  T.elements = dataLen / 4;
  T.error = SPARROW_TLV_ERROR_NO_ERROR;
  T.opcode = request->opcode;
  sparrow_tlv_replyify(&T);
  T.data = data;
  return sparrow_tlv_to_bytes(&T, reply, len);
}
/*---------------------------------------------------------------------------*/
