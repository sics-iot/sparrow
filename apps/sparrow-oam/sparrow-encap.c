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
 *         Encapsulation protocol implemention
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Oriol Piñol Piñol <oriol.pinol@yanzinetworks.com>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "sparrow-encap.h"
#include "lib/crc32.h"
#include <string.h>
#include <stdio.h>

#define CRC32_MAGIC_REMAINDER   0x2144DF1C

#define SHIFT_VERSION(x) (x << 4)

/*---------------------------------------------------------------------------*/
static inline uint8_t
verify_crc32(const uint8_t *buff, int len)
{
  if(CRC32_MAGIC_REMAINDER == crc32(buff, len)) {
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/*
 * Process encapsulation header.
 * write payload type to "payload_type" in non-null.
 * write payload length to "payload_len" in non-null.
 * Return number of bytes consumed by encapsulation, or (negative) error.
 */
int32_t
sparrow_encap_parse_and_verify(const uint8_t *p, size_t len, sparrow_encap_pdu_info_t *pinfo)
{
  int16_t retval = 4;

  pinfo->payload_len = 0;
  pinfo->dont_reply = 0;

  if(len < 4) {
    return SPARROW_ENCAP_ERROR_SHORT;
  }

  pinfo->version = (p[0] >> 4);
  /* WANNA REJECT HIGHER VERSIONS THAN 2 */
  if(pinfo->version > SPARROW_ENCAP_VERSION_MAX) {
    return SPARROW_ENCAP_ERROR_BAD_VERSION;
  }

  if((p[1] == 0x00) || (p[1] == 0x02) || (p[1] > SPARROW_ENCAP_PAYLOAD_MAX)) {
    return SPARROW_ENCAP_ERROR_BAD_PAYLOAD_TYPE;
  }

  if(p[2] != 0) {
    return SPARROW_ENCAP_ERROR_REQUEST_WITH_ERROR;
  }

  pinfo->fpmode = (p[3] >> 4) & 0x0f;
  if(pinfo->fpmode > SPARROW_ENCAP_FP_MODE_MAX) {
    return SPARROW_ENCAP_ERROR_BAD_FP_MODE;
  }

  pinfo->ivmode = p[3] & 0x0f;
  if(pinfo->ivmode > SPARROW_ENCAP_IVMODE_MAX) {
    return SPARROW_ENCAP_ERROR_BAD_IVMODE;
  }

  pinfo->fpmatchlen = SPARROW_ENCAP_FP_MATCHLEN_NONE;
  pinfo->fp = (uint8_t *)&p[4];
  switch(pinfo->fpmode) {
  case SPARROW_ENCAP_FP_MODE_NONE:
    pinfo->fplen = 0;
    pinfo->fp = NULL;
    break;
  case SPARROW_ENCAP_FP_MODE_DEVID:
    pinfo->fplen = 8;
    break;
  case SPARROW_ENCAP_FP_MODE_FIP4:
    return SPARROW_ENCAP_ERROR_BAD_FP_MODE;
  case SPARROW_ENCAP_FP_MODE_LENOPT:
    pinfo->fplen = 4;
    break;
  case SPARROW_ENCAP_FP_MODE_DID_AND_FP:
    return SPARROW_ENCAP_ERROR_BAD_FP_MODE;
  }
  if(pinfo->fpmatchlen == SPARROW_ENCAP_FP_MATCHLEN_NONE) {
    pinfo->fpmatchlen = pinfo->fplen;
  }
  pinfo->iv = (uint8_t *)&p[4 + pinfo->fplen];
  retval += pinfo->fplen;

  if(len < retval) {
    return SPARROW_ENCAP_ERROR_SHORT;
  }

  if(pinfo->fpmode == SPARROW_ENCAP_FP_MODE_LENOPT) {
    pinfo->payload_len = ((uint16_t)p[6] << 8) | p[7];
    if(p[5] == SPARROW_ENCAP_FP_LENOPT_OPTION_CRC
       || p[5] == SPARROW_ENCAP_FP_LENOPT_OPTION_SEQNO_CRC) {
      if(len < retval + pinfo->payload_len + 4) {
        return SPARROW_ENCAP_ERROR_SHORT;
      }
      if(!verify_crc32(p, len)) {
        return SPARROW_ENCAP_ERROR_BAD_CHECKSUM;
      }
    }
  }

  switch(pinfo->ivmode) {
  case SPARROW_ENCAP_IVMODE_128BIT:
    pinfo->ivlen = 16;
    break;
  case SPARROW_ENCAP_IVMODE_NONE:
    pinfo->ivlen = 0;
    break;
  }
  retval += pinfo->ivlen;

  if(len < retval) {
    return SPARROW_ENCAP_ERROR_SHORT;
  }

  pinfo->payload_type = p[1];

  if(pinfo->payload_len == 0) {
    pinfo->payload_len = len - retval;
  }
  return retval;
}
/*----------------------------------------------------------------*/

/*
 * Initialize a sparrow_encap_pdu_info_t for use when transmitting
 * events or other unsolicited TLV pdus.
 */
void
sparrow_encap_init_pdu_info_for_event(sparrow_encap_pdu_info_t *pinfo)
{
  const sparrow_id_t *did;
  if(pinfo == NULL) {
    return;
  }
  did = sparrow_get_did();

  pinfo->version = SPARROW_ENCAP_VERSION1;
  pinfo->fp = &did->id[8];
  pinfo->iv = NULL;
  pinfo->payload_len = 0;
  pinfo->payload_type = SPARROW_ENCAP_PAYLOAD_TLV;
  pinfo->fpmode = SPARROW_ENCAP_FP_MODE_DEVID;
  pinfo->fplen = 8;
  pinfo->ivmode = SPARROW_ENCAP_IVMODE_NONE;
  pinfo->ivlen = 0;
  pinfo->fpmatchlen = SPARROW_ENCAP_FP_MATCHLEN_NONE;
  pinfo->dont_reply = 0;
}
/*----------------------------------------------------------------*/

/*
 * Write encapsulation header to "buf", no more than "len" bytes and
 * return number of bytes written.
 *
 * "pinfo" should be filled-in by sparrow_encap_parse_and_verify()
 * finger print and init vector pointers within "pinfo" are changed to
 * point to within "buf".
 */
size_t
sparrow_encap_write_header_with_error(uint8_t *buf, size_t len, sparrow_encap_pdu_info_t *pinfo, uint8_t error)
{
  int32_t retval;

  if(pinfo == NULL) {
    return 0;
  }

  retval = 4; /* encap header */
  retval += pinfo->fplen; /* finger print */
  retval += pinfo->ivlen; /* init vector */

  if(len < retval) {
    return 0;
  }

  buf[0] = SHIFT_VERSION(pinfo->version);
  buf[1] = pinfo->payload_type;
  buf[2] = error;

  buf[3] = pinfo->fpmode << 4;
  /* copy finger print */
  if(pinfo->fplen > 0) {
    if(pinfo->fp != NULL) {
      memcpy(&buf[4], pinfo->fp, pinfo->fplen);
    }
  }
  /* set finger print pointer in pinfo */
  pinfo->fp = &buf[4];

  buf[3] |= (pinfo->ivmode & 0x0f);
  if(pinfo->ivlen > 0) {
    if(pinfo->iv == NULL) {
      sparrow_random_fill(&buf[8], pinfo->ivlen);
      pinfo->iv = &buf[4 + pinfo->fplen];
    } else {
      memcpy(&buf[4 + pinfo->fplen], pinfo->iv, pinfo->ivlen);
    }
  }
  pinfo->iv = &buf[4 + pinfo->fplen];

  return retval;
}
/*----------------------------------------------------------------*/

/*
 * Do any finalization if needed.
 *
 * Return length of pdu, or 0 on failure to finalize a pdu.
 */
size_t
sparrow_encap_finalize(uint8_t *buf, size_t maxlen,
                                   sparrow_encap_pdu_info_t *pinfo,
                                   size_t pdulen)
{
  return pdulen;
}
/*----------------------------------------------------------------*/
