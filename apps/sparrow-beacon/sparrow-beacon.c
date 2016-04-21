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
 *
 */

/**
 * \file
 *         Sparrow beacon format implemention
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include <string.h>

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define BEACON_TLV_TYPE_6LOWPAN   1
#define BEACON_TLV_TYPE_OUI       2
#define BEACON_TLV_TYPE_LOCATION  3
#define BEACON_TLV_TYPE_ETX       5

#define BEACON_TLV_OUI_TYPE_LOCATION 1
#define BEACON_TLV_OUI_TYPE_OWNER    2
#define BEACON_TLV_OUI_TYPE_ADDRESS  3

/*---------------------------------------------------------------------------*/

 /*
  * Get a value from a beacon payload identified by the "type_len"
  * number of bytes first in "type", and write it into "dst", no more
  * than "dst_len" bytes.
  *
  * Example: to get Yanzi location id search for {0x02, 0x00, 0x90, 0xda, 0x02 } type_len 5.
  *
  * Beacon payload example: 02010a020090da0100010cfa0a020090da020000126700
  * Return number of bytes written.
 */
uint8_t
sparrow_beacon_get_value_from_beacon(const uint8_t *beacon, uint8_t beacon_len,
                                     const uint8_t *type, uint8_t type_len,
                                     uint8_t *dst, uint8_t dst_len)
{
  int i = 0;
  size_t copy_len;

  if(beacon[0] == 0xfe) {
    i = 1;
  }
  for( ; (beacon[i] > 0) && (i + beacon[i] < beacon_len); i += beacon[i]) {
    if(beacon[i] <= type_len) {
      continue;
    }
    if(memcmp(&beacon[i + 1], type, type_len) != 0) {
      continue;
    }
    copy_len = beacon[i] - type_len -1;
    if(dst_len < copy_len) {
      copy_len = dst_len;
    }
    if(copy_len > 0) {
      memcpy(dst, &beacon[i + 1 + type_len], copy_len);
      return copy_len;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
uint8_t
sparrow_beacon_get_owner(const uint8_t *bp, uint8_t len,
                         uint8_t *dst, uint8_t dst_len)
{
  uint8_t type[5];
  type[0] = BEACON_TLV_TYPE_OUI;
  type[1] = 0x00;
  type[2] = 0x90;
  type[3] = 0xda;
  type[4] = BEACON_TLV_OUI_TYPE_OWNER;
  return sparrow_beacon_get_value_from_beacon(bp, len, type, 5, dst, dst_len);
}
/*---------------------------------------------------------------------------*/
uint8_t
sparrow_beacon_get_address(const uint8_t *bp, uint8_t len,
                           uint8_t *dst, uint8_t dst_len)
{
  uint8_t type[5];
  type[0] = BEACON_TLV_TYPE_OUI;
  type[1] = 0x00;
  type[2] = 0x90;
  type[3] = 0xda;
  type[4] = BEACON_TLV_OUI_TYPE_ADDRESS;
  return sparrow_beacon_get_value_from_beacon(bp, len, type, 5, dst, dst_len);
}
/*---------------------------------------------------------------------------*/
uint8_t
sparrow_beacon_get_etx(const uint8_t *bp, uint8_t len,
                       uint8_t *dst, uint8_t dst_len)
{
  uint8_t type = BEACON_TLV_TYPE_ETX;
  return sparrow_beacon_get_value_from_beacon(bp, len, &type, 1, dst, dst_len);
}
/*---------------------------------------------------------------------------*/
uint8_t
sparrow_beacon_get_location(const uint8_t *bp, uint8_t len,
                            uint8_t *dst, uint8_t dst_len)
{
  uint8_t type[5];
  uint8_t type_len;
  if(dst_len == 4) {
    type[0] = BEACON_TLV_TYPE_OUI;
    type_len = 5;
  } else {
    type[0] = BEACON_TLV_TYPE_LOCATION;
    type_len = 1;
  }
  type[1] = 0x00;
  type[2] = 0x90;
  type[3] = 0xda;
  type[4] = BEACON_TLV_OUI_TYPE_LOCATION;
  return sparrow_beacon_get_value_from_beacon(bp, len, type, type_len, dst, dst_len);
}

/*---------------------------------------------------------------------------*/
/*
 * Calculate beacon payload length.
 * Return length or -1 on error.
 */
int
sparrow_beacon_calculate_payload_length(const uint8_t *beacon, uint8_t maxlen)
{
  int i;

  if(beacon[0] != 0xfe) {
    return -1;
  }

  for(i = 1; beacon[i] != 0; i += beacon[i]) {
    if(i >= maxlen) {
      return -1;
    }
  }
  i++; /* include the last 0-length indicator */
  return i;
}
/*---------------------------------------------------------------------------*/
