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

#ifndef SPARROW_TLV_H_
#define SPARROW_TLV_H_

#include "sparrow.h"

#define SPARROW_TLV_VERSION               0
#define SPARROW_TLV_OPCODE_VECTOR_MASK 0x80
#define SPARROW_TLV_OPCODE_REPLY_MASK  0x01

typedef enum {
  SPARROW_TLV_OPCODE_GET_REQUEST                     = 0x00,
  SPARROW_TLV_OPCODE_GET_RESPONSE                    = 0x01,
  SPARROW_TLV_OPCODE_SET_REQUEST                     = 0x02,
  SPARROW_TLV_OPCODE_SET_RESPONSE                    = 0x03,
  SPARROW_TLV_OPCODE_REFLECT_REQUEST                 = 0x04,
  SPARROW_TLV_OPCODE_REFLECT_RESPONSE                = 0x05,
  SPARROW_TLV_OPCODE_EVENT_REQUEST                   = 0x06,
  SPARROW_TLV_OPCODE_EVENT_RESPONSE                  = 0x07,
  SPARROW_TLV_OPCODE_MASKED_SET_REQUEST              = 0x08,
  SPARROW_TLV_OPCODE_MASKED_SET_RESPONSE             = 0x09,
  SPARROW_TLV_OPCODE_ABORT_IF_EQUAL_REQUEST          = 0x0a,
  SPARROW_TLV_OPCODE_ABORT_IF_EQUAL_RESPONSE         = 0x0b,
  SPARROW_TLV_OPCODE_BLOB_REQUEST                    = 0x0c,
  SPARROW_TLV_OPCODE_BLOB_RESPONSE                   = 0x0d,
  SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST              = 0x80,
  SPARROW_TLV_OPCODE_VECTOR_GET_RESPONSE             = 0x81,
  SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST              = 0x82,
  SPARROW_TLV_OPCODE_VECTOR_SET_RESPONSE             = 0x83,
  SPARROW_TLV_OPCODE_VECTOR_REFLECT_REQUEST          = 0x84,
  SPARROW_TLV_OPCODE_VECTOR_REFLECT_RESPONSE         = 0x85,
  SPARROW_TLV_OPCODE_VECTOR_EVENT_REQUEST            = 0x86,
  SPARROW_TLV_OPCODE_VECTOR_EVENT_RESPONSE           = 0x87,
  SPARROW_TLV_OPCODE_VECTOR_MASKED_SET_REQUEST       = 0x88,
  SPARROW_TLV_OPCODE_VECTOR_MASKED_SET_RESPONSE      = 0x89,
  SPARROW_TLV_OPCODE_VECTOR_ABORT_IF_EQUAL_REQUEST   = 0x8a,
  SPARROW_TLV_OPCODE_VECTOR_ABORT_IF_EQUAL_RESPONSE  = 0x8b,
  SPARROW_TLV_OPCODE_VECTOR_BLOB_REQUEST             = 0x8c,
  SPARROW_TLV_OPCODE_VECTOR_BLOB_RESPONSE            = 0x8d,
} sparrow_tlv_opcode_t;

typedef enum {
  SPARROW_TLV_ERROR_NO_ERROR                         = 0,
  SPARROW_TLV_ERROR_UNKNOWN_VERSION                  = 1,
  SPARROW_TLV_ERROR_UNKNOWN_VARIABLE                 = 2,
  SPARROW_TLV_ERROR_UNKNOWN_INSTANCE                 = 3,
  SPARROW_TLV_ERROR_UNKNOWN_OP_CODE                  = 4,
  SPARROW_TLV_ERROR_UNKNOWN_ELEMENT_SIZE             = 5,
  SPARROW_TLV_ERROR_BAD_NUMBER_OF_ELEMENTS           = 6,
  SPARROW_TLV_ERROR_TIMEOUT                          = 7,
  SPARROW_TLV_ERROR_DEVICE_BUSY                      = 8,
  SPARROW_TLV_ERROR_HARDWARE_ERROR                   = 9,
  SPARROW_TLV_ERROR_BAD_LENGTH                       = 10,
  SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED              = 11,
  SPARROW_TLV_ERROR_UNKNOWN_BLOB_COMMAND             = 12,
  SPARROW_TLV_ERROR_NO_VECTOR_ACCESS                 = 13,
  SPARROW_TLV_ERROR_UNEXPECTED_RESPONSE              = 14,
  SPARROW_TLV_ERROR_INVALID_VECTOR_OFFSET            = 15,
  SPARROW_TLV_ERROR_INVALID_ARGUMENT                 = 16,
  SPARROW_TLV_ERROR_READ_ACCESS_DENIED               = 17,
  SPARROW_TLV_ERROR_UNPROCESSED_TLV                  = 18,
} sparrow_tlv_error_t;

typedef struct sparrow_tlv {
  uint32_t offset;
  uint32_t elements;
  const uint8_t *data;
  uint16_t length;
  uint16_t variable;
  uint8_t version;
  uint8_t instance;
  uint8_t opcode;
  uint8_t element_size;
  uint8_t error;
} sparrow_tlv_t;

extern const uint8_t sparrow_tlv_zeroes[16];

/*
 * Initialize the TLV as a 32 bit get request.
 */
void sparrow_tlv_init_get32(sparrow_tlv_t *t, uint8_t instance, uint16_t variable);

/*
 * Initialize the TLV as a 64 bit get request.
 */
void sparrow_tlv_init_get64(sparrow_tlv_t *t, uint8_t instance, uint16_t variable);

/*
 * Print the TLV if debug output is enabled.
 */
void sparrow_tlv_print(const sparrow_tlv_t *tlv);

/*
 * Return length of TLV pointed to by "p", in number of bytes.
 */
size_t sparrow_tlv_length_from_data(const uint8_t *p);

/*
 * Return true if this opcode is carrying data.
 */
uint8_t sparrow_tlv_with_data(sparrow_tlv_opcode_t o);

/*
 * Check if this TLV contains a discovery variable request or
 * response.
 */
uint8_t sparrow_tlv_is_discovery_variable(const sparrow_tlv_t *t);

/*
 * Set reply bit on opcode in TLV pointed to by "t".
 */
void sparrow_tlv_replyify(sparrow_tlv_t *t);

/*
 * Combine 2 bytes of data from an array to a host byte order int 16.
 */
uint16_t sparrow_tlv_get_int16_from_data(const uint8_t *p);

/*
 * Write 16 bit host byte order value to byte array in network byte order.
 */
void sparrow_tlv_write_int16_to_buf(uint8_t *p, uint16_t d);

/*
 * Combine 4 bytes of data from an array to a host byte order int 32.
 */
uint32_t sparrow_tlv_get_int32_from_data(const uint8_t *p);

/*
 * Write 32 bit host byte order value to byte array in network byte order.
 */
void sparrow_tlv_write_int32_to_buf(uint8_t *p, uint32_t d);

/*
 * Combine 8 bytes of data from an array to a host byte order int 64.
 */
uint64_t sparrow_tlv_get_int64_from_data(const uint8_t *p);

/*
 * Write 64 bit host byte order value to byte array in network byte order.
 */
void sparrow_tlv_write_int64_to_buf(uint8_t *p, uint64_t d);

/*
 * Parse a TLV in a byte array.
 * Note that length and elementsize fields are converted to indicate length in bytes.
 * Data (if any) is not copied, tlv->data is set to point to it.
 * Return number of bytes consumed by this TLV.
 */
size_t sparrow_tlv_from_bytes(sparrow_tlv_t *t, const uint8_t *in_data);

/*
 * Write a TLV, including copy the data (if any), to a destination
 * buffer, no more than "len" bytes.
 *
 * Return number of bytes written.
 */
size_t sparrow_tlv_to_bytes(const sparrow_tlv_t *t, uint8_t *data, size_t len);

/*
 * Construct an error TLV with the given error, without data. All
 * other fields are taken from "request".
 * Error TLV is written to "reply", no more than "len" bytes.
 * Return number of bytes written.
 */
size_t sparrow_tlv_write_reply_error(const sparrow_tlv_t *request,
                                     uint8_t error,
                                     uint8_t *reply, size_t len);

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and
 * should already be in network byte order.
 */
size_t sparrow_tlv_write_reply32(const sparrow_tlv_t *request,
                                 uint8_t *reply, size_t len,
                                 const uint8_t *data);

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and is
 * converted to network byte order.
 */
size_t sparrow_tlv_write_reply32int(const sparrow_tlv_t *request,
                                    uint8_t *reply, size_t len, uint32_t data);

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and
 * should already be in network byte order.
 */
size_t sparrow_tlv_write_reply64(const sparrow_tlv_t *request,
                                 uint8_t *reply, size_t len,
                                 const uint8_t *data);

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes", with payload "data". Data is converted to
 * network byte order.
 */
size_t sparrow_tlv_write_reply64int(const sparrow_tlv_t *request,
                                    uint8_t *reply, size_t len, uint64_t data);

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and
 * should already be in network byte order.
 */
size_t sparrow_tlv_write_reply128(const sparrow_tlv_t *request,
                                  uint8_t *reply, size_t len,
                                  const uint8_t *data);

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and
 * should already be in network byte order.
 */
size_t sparrow_tlv_write_reply256(const sparrow_tlv_t *request,
                                  uint8_t *reply, size_t len,
                                  const uint8_t *data);

/*
 * Write a non-vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data", and
 * should already be in network byte order.
 */
size_t sparrow_tlv_write_reply512(const sparrow_tlv_t *request,
                                  uint8_t *reply, size_t len,
                                  const uint8_t *data);

/*
 * Write a vector reply TLV based on "request" to "reply", no more
 * than "len" bytes".  The payload data is read from "data" with
 * offset and elements from request.  No byteorder conversion is done.
 */
size_t sparrow_tlv_write_reply_vector(const sparrow_tlv_t *request,
                                      uint8_t *reply, size_t len,
                                      const uint8_t *data);

/*
 * Write a non-vector Blob reply TLV based on "request" to "reply", no
 * more than "len" bytes".
 *
 * The payload data is read from "data", "dataLen" number of bytes,
 * and should already be in network byte order. Data should already be
 * padded with zero to n*4 bytes.
 */
size_t sparrow_tlv_write_reply_blob(const sparrow_tlv_t *request,
                                    uint8_t *reply, size_t len,
                                    const uint8_t *data, size_t data_len);

#endif /* SPARROW_TLV_H_ */
