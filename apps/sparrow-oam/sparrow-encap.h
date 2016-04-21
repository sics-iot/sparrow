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
 *         Encapsulation protocol implemention
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Oriol Piñol Piñol <oriol.pinol@yanzinetworks.com>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef SPARROW_ENCAP_H_
#define SPARROW_ENCAP_H_

#include "sparrow.h"
#include <stdint.h>

#define SPARROW_ENCAP_VERSION1                       1
#define SPARROW_ENCAP_VERSION2                       2
#define SPARROW_ENCAP_VERSION_MAX                    2

#define SPARROW_ENCAP_PAYLOAD_TLV                    1
#define SPARROW_ENCAP_PAYLOAD_PORTAL_SELECTION_PROTO 3
#define SPARROW_ENCAP_PAYLOAD_ECHO_REQUEST           4
#define SPARROW_ENCAP_PAYLOAD_ECHO_REPLY             5
#define SPARROW_ENCAP_PAYLOAD_STOP_TRANSMITTER       6
#define SPARROW_ENCAP_PAYLOAD_START_TRANSMITTER      7
#define SPARROW_ENCAP_PAYLOAD_SERIAL                 8
#define SPARROW_ENCAP_PAYLOAD_DEBUG                  9
#define SPARROW_ENCAP_PAYLOAD_SERIAL_WITH_SEQNO     10
#define SPARROW_ENCAP_PAYLOAD_RECEIVE_REPORT        11
#define SPARROW_ENCAP_PAYLOAD_SLEEP_REPORT          12
#define SPARROW_ENCAP_PAYLOAD_SILENTLY_DISCARD      13
#define SPARROW_ENCAP_PAYLOAD_MAX                   13

#define SPARROW_ENCAP_ERROR_OK                       0
#define SPARROW_ENCAP_ERROR_SHORT                   -1
#define SPARROW_ENCAP_ERROR_BAD_VERSION             -2
#define SPARROW_ENCAP_ERROR_BAD_PAYLOAD_TYPE        -3
#define SPARROW_ENCAP_ERROR_REQUEST_WITH_ERROR      -4
#define SPARROW_ENCAP_ERROR_BAD_FP_MODE             -5
#define SPARROW_ENCAP_ERROR_BAD_IVMODE              -6
#define SPARROW_ENCAP_ERROR_UNKNOWN_KEY             -7
#define SPARROW_ENCAP_ERROR_BAD_SEQUENCE            -8
#define SPARROW_ENCAP_ERROR_BAD_CHECKSUM            -9

typedef enum {
  SPARROW_ENCAP_FP_MODE_NONE                       = 0, /* 0 bytes */
  SPARROW_ENCAP_FP_MODE_DEVID                      = 1, /* 8 bytes */
  SPARROW_ENCAP_FP_MODE_FIP4                       = 2, /* 4 bytes */
  SPARROW_ENCAP_FP_MODE_LENOPT                     = 3, /* 4 bytes */
  SPARROW_ENCAP_FP_MODE_DID_AND_FP                 = 4, /* 16 bytes */
  SPARROW_ENCAP_FP_MODE_MAX                        = 4,
} sparrow_encap_fp_mode_t;

#define SPARROW_ENCAP_FP_LENOPT_OPTION_CRC       1
#define SPARROW_ENCAP_FP_LENOPT_OPTION_SEQNO_CRC 2

typedef enum {
  SPARROW_ENCAP_IVMODE_NONE   = 0,
  SPARROW_ENCAP_IVMODE_128BIT = 1,
  SPARROW_ENCAP_IVMODE_MAX    = 1,
} sparrow_encap_ivmode_t;

#define SPARROW_ENCAP_FP_MATCHLEN_NONE 0xff

typedef struct {
  uint8_t version;
  const uint8_t *fp;
  const uint8_t *iv;
  size_t payload_len;
  uint8_t payload_type;
  sparrow_encap_fp_mode_t fpmode;
  sparrow_encap_ivmode_t ivmode;
  uint8_t fplen;
  uint8_t ivlen;
  uint8_t fpmatchlen;
  uint8_t dont_reply;
} sparrow_encap_pdu_info_t;

int32_t sparrow_encap_parse_and_verify(const uint8_t *p, size_t len, sparrow_encap_pdu_info_t *info);

size_t sparrow_encap_write_header_with_error(uint8_t *buf, size_t len, sparrow_encap_pdu_info_t *info, uint8_t error);

static inline size_t
sparrow_encap_write_header(uint8_t *buf, size_t len, sparrow_encap_pdu_info_t *info)
{
  return sparrow_encap_write_header_with_error(buf, len, info, SPARROW_ENCAP_ERROR_OK);
}

size_t sparrow_encap_finalize(uint8_t *buf, size_t maxlen, sparrow_encap_pdu_info_t *pinfo, size_t pdulen);

void sparrow_encap_init_pdu_info_for_event(sparrow_encap_pdu_info_t *pinfo);

#endif /* SPARROW_ENCAP_H_ */
