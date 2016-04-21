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

#ifndef PACKETUTILS_H_
#define PACKETUTILS_H_

#include "contiki-conf.h"

/**
 * The radio params and packet attributes are serialized between
 * native border router and serial radio. They are mapped to
 * packetutil specific constants to ensure they do not change
 * regardless of changes in Contiki core.
 *
 * Note these are the internal serialization types and should not be
 * used by application code.
 */

enum {
  PACKETUTILS_RADIO_PARAM_CHANNEL,
  PACKETUTILS_RADIO_PARAM_PANID,
  PACKETUTILS_RADIO_PARAM_SHORT_LLADDR,
  PACKETUTILS_RADIO_PARAM_LLADDR0,
  PACKETUTILS_RADIO_PARAM_LLADDR1,
  PACKETUTILS_RADIO_PARAM_LLADDR2,
  PACKETUTILS_RADIO_PARAM_LLADDR3,
  PACKETUTILS_RADIO_PARAM_RX_MODE,
  PACKETUTILS_RADIO_PARAM_RSSI,
  PACKETUTILS_RADIO_PARAM_TXPOWER,
  PACKETUTILS_RADIO_PARAM_TX_MODE,
  PACKETUTILS_RADIO_PARAM_CCA_THRESHOLD,
  PACKETUTILS_RADIO_MAX
};

enum {
  PACKETUTILS_ATTR_NONE,

  PACKETUTILS_ATTR_CHANNEL,
  PACKETUTILS_ATTR_LINK_QUALITY,
  PACKETUTILS_ATTR_RSSI,
  PACKETUTILS_ATTR_TIMESTAMP,
  PACKETUTILS_ATTR_RADIO_TXPOWER,
  PACKETUTILS_ATTR_LISTEN_TIME,
  PACKETUTILS_ATTR_TRANSMIT_TIME,
  PACKETUTILS_ATTR_MAX_MAC_TRANSMISSIONS,
  PACKETUTILS_ATTR_MAC_SEQNO,
  PACKETUTILS_ATTR_MAC_ACK,

  PACKETUTILS_ATTR_MAX
};

int8_t packetutils_from_radio_param(int radio_param);
int packetutils_to_radio_param(int8_t radio_param);

int packetutils_serialize_atts(uint8_t *data, int size);

int packetutils_deserialize_atts(const uint8_t *data, int size);

int packetutils_deserialize_packetbuf(const uint8_t *data, int len);
int packetutils_serialize_packetbuf(uint8_t *data, int size);

#endif /* PACKETUTILS_H_ */
