/*
 * Copyright (c) 2011-2016, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "contiki.h"
#include "packetutils.h"
#include "dev/radio.h"
#include "net/packetbuf.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

/**
 * NOTE: Never ever change the order in the map tables because that
 * would break backward compatibility! Support for any new attributes
 * must be added last and PACKETUTILS_ATTR_MAX/PACKETUTILS_RADIO_MAX
 * updated.
 */
/*---------------------------------------------------------------------------*/
static const uint8_t ATTR_MAP[PACKETUTILS_ATTR_MAX] = {
  PACKETBUF_ATTR_NONE,                    /* PACKETUTILS_ATTR_NONE */

  PACKETBUF_ATTR_CHANNEL,                 /* PACKETUTILS_ATTR_CHANNEL */
  PACKETBUF_ATTR_LINK_QUALITY,            /* PACKETUTILS_ATTR_LINK_QUALITY */
  PACKETBUF_ATTR_RSSI,                    /* PACKETUTILS_ATTR_RSSI */
  PACKETBUF_ATTR_TIMESTAMP,               /* PACKETUTILS_ATTR_TIMESTAMP */
  PACKETBUF_ATTR_RADIO_TXPOWER,           /* PACKETUTILS_ATTR_RADIO_TXPOWER */
  PACKETBUF_ATTR_LISTEN_TIME,             /* PACKETUTILS_ATTR_LISTEN_TIME */
  PACKETBUF_ATTR_TRANSMIT_TIME,           /* PACKETUTILS_ATTR_TRANSMIT_TIME */
  PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS,   /* PACKETUTILS_ATTR_MAX_MAC_TRANSMISSIONS */
  PACKETBUF_ATTR_MAC_SEQNO,               /* PACKETUTILS_ATTR_MAC_SEQNO */
  PACKETBUF_ATTR_MAC_ACK,                 /* PACKETUTILS_ATTR_MAC_ACK */
};
/*---------------------------------------------------------------------------*/
static const int8_t RADIO_MAP[PACKETUTILS_RADIO_MAX] = {
  RADIO_PARAM_CHANNEL,          /* PACKETUTILS_RADIO_PARAM_CHANNEL */
  RADIO_PARAM_PAN_ID,           /* PACKETUTILS_RADIO_PARAM_PANID */
  RADIO_PARAM_16BIT_ADDR,       /* PACKETUTILS_RADIO_PARAM_SHORT_LLADDR */
  -1,                           /* PACKETUTILS_RADIO_PARAM_LLADDR0 */
  -1,                           /* PACKETUTILS_RADIO_PARAM_LLADDR1 */
  -1,                           /* PACKETUTILS_RADIO_PARAM_LLADDR2 */
  -1,                           /* PACKETUTILS_RADIO_PARAM_LLADDR3 */
  RADIO_PARAM_RX_MODE,          /* PACKETUTILS_RADIO_PARAM_RX_MODE */
  RADIO_PARAM_RSSI,             /* PACKETUTILS_RADIO_PARAM_RSSI */
  RADIO_PARAM_TXPOWER,          /* PACKETUTILS_RADIO_PARAM_TXPOWER */
  RADIO_PARAM_TX_MODE,          /* PACKETUTILS_RADIO_PARAM_TX_MODE */
  RADIO_PARAM_CCA_THRESHOLD,    /* PACKETUTILS_RADIO_PARAM_CCA_THRESHOLD */
};
/*---------------------------------------------------------------------------*/
int8_t
packetutils_from_radio_param(int radio_param)
{
  int i;
  for(i = 0; i < PACKETUTILS_RADIO_MAX; i++) {
    if(RADIO_MAP[i] == radio_param) {
      return i;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int
packetutils_to_radio_param(int8_t radio_param)
{
  if(radio_param < 0 || radio_param >= PACKETUTILS_RADIO_MAX) {
    return -1;
  }
  return RADIO_MAP[radio_param];
}
/*---------------------------------------------------------------------------*/
int
packetutils_serialize_atts(uint8_t *data, int size)
{
  int i;
  /* set the length first later */
  int pos = 1;
  int cnt = 0;
  /* assume that values are 16-bit */
  uint16_t val;
  PRINTF("packetutils: serializing packet atts");
  for(i = 0; i < PACKETUTILS_ATTR_MAX; i++) {
    val = packetbuf_attr(ATTR_MAP[i]);
    if(val != 0) {
      if(pos + 3 > size) {
        return -1;
      }
      data[pos++] = i;
      data[pos++] = val >> 8;
      data[pos++] = val & 255;
      cnt++;
      PRINTF(" %d=%d", i, val);
    }
  }
  PRINTF(" (%d)\n", cnt);

  data[0] = cnt;
  return pos;
}
/*---------------------------------------------------------------------------*/
int
packetutils_deserialize_atts(const uint8_t *data, int size)
{
  int i, cnt, pos, attr;

  pos = 0;
  cnt = data[pos++];
  PRINTF("packetutils: deserializing %d packet atts:", cnt);
  if(size < cnt * 3 + pos) {
    PRINTF(" *** too many. size=%d!\n", size);
    return -1;
  }
  for(i = 0; i < cnt; i++, pos += 3) {
    if(data[pos] >= PACKETUTILS_ATTR_MAX) {
      PRINTF(" *** ignoring unknown attribute %u\n", data[pos]);
      /* Continue with next attribute for backward compatibility */
      continue;
    }
    attr = ATTR_MAP[data[pos]];
    PRINTF(" %d=%d", attr, (data[pos + 1] << 8) | data[pos + 2]);
    packetbuf_set_attr(attr, (data[pos + 1] << 8) | data[pos + 2]);
  }
  PRINTF("\n");
  return pos;
}
/*---------------------------------------------------------------------------*/
int
packetutils_serialize_packetbuf(uint8_t *data, int size)
{
  int n;

  n = packetutils_serialize_atts(data, size);
  /* TODO: add packet data also!!! */

  return n;
}
/*---------------------------------------------------------------------------*/
int
packetutils_deserialize_packetbuf(const uint8_t *data, int len)
{
  int pos;
  packetbuf_clear();
  pos = packetutils_deserialize_atts(data, len);
  if(pos < 0) {
    return 0;
  }
  len -= pos;
  if(len > PACKETBUF_SIZE) {
    len = PACKETBUF_SIZE;
  }
  memcpy(packetbuf_dataptr(), &data[pos], len);
  packetbuf_set_datalen(len);

  return pos;
}
/*---------------------------------------------------------------------------*/
