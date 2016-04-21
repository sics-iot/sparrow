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

/**
 * \file
 *         Encap packet handling for the serial-radio.
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"
#include "sys/cc.h"
#include "net/netstack.h"
#include "net/ip/uip.h"
#include "net/packetbuf.h"
#include "packetutils.h"
#include "dev/slip.h"
#include "lib/crc32.h"
#include "serial-radio.h"
#include "serial-radio-stats.h"
#include "dev/radio-802154-dev.h"
#include "sparrow-encap.h"
#include "enc-net.h"
#include <stdio.h>

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

void serial_radio_input(sparrow_encap_pdu_info_t *pinfo, uint8_t *data, int enclen);

static void enc_net_input(void);

extern uint8_t verbose_output;
extern uint32_t border_router_api_version;

#define WITH_FLOW_CONTROL 0
#if WITH_FLOW_CONTROL
static uint8_t start_transmitt_data[4] = { 0, 0, 0, 1 };
static struct timer start_transmitt_timer;
#endif /* WITH_FLOW_CONTROL */

static uint32_t last_received_seqno;
static uint32_t last_sent_seqno;
static uint32_t next_seqno;

static uint32_t last_packet_seqno = 0;
static uint16_t last_packet_len = 0;
static uint8_t last_packet_data[1];
static uint8_t is_using_ack = 0;
static uint8_t is_using_nack = 0;
static uint8_t has_received_seqno = 0;

static struct timer send_timer;
static int debug_frame = 0;

static uint16_t slip_fragment_delay = 1000;
static uint16_t slip_fragment_size = 62;

#define TIMED_RESEND 1
#if TIMED_RESEND
static struct ctimer resend_timer;
#endif
/*---------------------------------------------------------------------------*/
#define WRITE_STATUS_OK 1

#define WRITEB(b) writeb(b)
static CC_INLINE int
writeb(unsigned char b)
{
  slip_arch_writeb(b);
  return WRITE_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
int
slip_writeb(unsigned char c)
{
  static int send_count = 0;
  int status;

  if(slip_fragment_delay == 0 || timer_expired(&send_timer)) {
    send_count = 0;
  }
  send_count++;
  status = WRITEB(c);
  if(send_count >= slip_fragment_size && slip_fragment_size > 0) {
    send_count = 0;
    clock_delay_usec(slip_fragment_delay);
  }
  if(slip_fragment_delay > 0) {
    timer_set(&send_timer, slip_fragment_delay / 1000 + 1);
  }
  return status;
}
/*--------------------------------------------------------------------------*/
void
enc_net_set_fragment_size(uint16_t size)
{
  slip_fragment_size = size;
}
/*--------------------------------------------------------------------------*/
void
enc_net_set_fragment_delay(uint16_t delay)
{
  slip_fragment_delay = delay;
}
/*--------------------------------------------------------------------------*/
static int
slip_frame_start(void)
{
  if(debug_frame) {
    debug_frame = 0;
    return slip_writeb(SLIP_END);
  }
  return WRITE_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
static int
slip_frame_end(void)
{
  int status;
  status = slip_writeb(SLIP_END);
  debug_frame = 0;
  return status;
}
/*---------------------------------------------------------------------------*/
#if !SERIAL_RADIO_CONF_NO_PUTCHAR
#undef putchar
int
putchar(int c)
{
  if(!debug_frame) {            /* Start of debug output */
    if(slip_writeb(SLIP_END) != WRITE_STATUS_OK) {
      debug_frame = 2;
    } else {
      slip_writeb('\r');     /* Type debug line == '\r' */
      debug_frame = 1;
    }
  }

  if(debug_frame == 1) {
    /* Need to also print '\n' because for example COOJA will not show
       any output before line end */
    slip_writeb((char)c);
  }

  /*
   * Line buffered output, a newline marks the end of debug output and
   * implicitly flushes debug output.
   */
  if(c == '\n') {
    if(debug_frame == 1) {
      slip_writeb(SLIP_END);
    }
    debug_frame = 0;
  }
  return c;
}
#endif /* SERIAL_RADIO_CONF_NO_PUTCHAR */
/*---------------------------------------------------------------------------*/
uint32_t
serial_get_mode(void)
{
  uint32_t v = 0;
  if(is_using_ack) {
    v |= SERIAL_MODE_ACK;
  }
  if(is_using_nack) {
    v |= SERIAL_MODE_NACK;
  }
  return v;
}
/*---------------------------------------------------------------------------*/
void
serial_set_mode(uint32_t mode)
{
  if(mode == SERIAL_MODE_ACK) {
    is_using_ack = 1;
  } else {
    is_using_ack = 0;
  }
}
/*---------------------------------------------------------------------------*/
static void
encnet_init(void)
{
#ifndef BAUD2UBR
#define BAUD2UBR(baud) baud
#endif
  slip_arch_init(BAUD2UBR(115200));
  process_start(&slip_process, NULL);
  slip_set_input_callback(enc_net_input);
}
/*---------------------------------------------------------------------------*/
static int
send_to_slip(const uint8_t *buf, int len)
{
  int i, s;
  uint8_t c;
  for(i = 0; i < len; ++i) {
    c = buf[i];
    if(c == SLIP_END) {
      s = slip_writeb(SLIP_ESC);
      if(s != WRITE_STATUS_OK) {
        return s;
      }
      c = SLIP_ESC_END;
    } else if(c == SLIP_ESC) {
      s = slip_writeb(SLIP_ESC);
      if(s != WRITE_STATUS_OK) {
        return s;
      }
      c = SLIP_ESC_ESC;
    }
    s = slip_writeb(c);
    if(s != WRITE_STATUS_OK) {
      return s;
    }
  }
  return WRITE_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
#if TIMED_RESEND
static void
resend_callback(void *ignore)
{
  if(last_packet_len > 0) {
    slip_frame_start();
    send_to_slip(last_packet_data, last_packet_len);
    slip_frame_end();

    /* At most one retransmission */
    last_packet_len = 0;
  }
}
#endif /* TIMED_RESEND */
/*---------------------------------------------------------------------------*/
static void
send_report(void)
{
  uint8_t buf[4];
  buf[0] = (last_received_seqno >> 24) & 0xff;
  buf[1] = (last_received_seqno >> 16) & 0xff;
  buf[2] = (last_received_seqno >> 8) & 0xff;
  buf[3] = last_received_seqno & 0xff;
  enc_net_send_packet_payload_type(buf, 4, SPARROW_ENCAP_PAYLOAD_RECEIVE_REPORT);
}
/*---------------------------------------------------------------------------*/
static void
enc_net_input(void)
{
  uint32_t seqno;
  int payload_len, enclen;
  sparrow_encap_pdu_info_t pinfo;
  uint8_t *payload;

  payload = uip_buf;
  payload_len = uip_len;
  uip_len = 0;
  uip_ext_len = 0;

  SERIAL_RADIO_STATS_ADD(SERIAL_RADIO_STATS_ENCAP_RECV, payload_len);

#if WITH_FLOW_CONTROL
  /* Signal that we can receive another packet */
  enc_net_send_packet_payload_type(start_transmitt_data,
                                sizeof(start_transmitt_data),
                                SPARROW_ENCAP_PAYLOAD_START_TRANSMITTER);
  timer_set(&start_transmitt_timer, CLOCK_SECOND / 16);
#endif /* WITH_FLOW_CONTROL */

  /* decode packet data - encap header + crc32 check */
  enclen = sparrow_encap_parse_and_verify(payload, payload_len, &pinfo);

  if(enclen == SPARROW_ENCAP_ERROR_BAD_CHECKSUM) {
    if(verbose_output) {
      printf("Packet input failed Bad CRC, len:%d, enclen:%d\n", payload_len, enclen);
    }
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_ERRORS);
    if(has_received_seqno && is_using_nack) {
      send_report();
    }
    return;
  }

  if(enclen < 0) {
    if(verbose_output) {
      printf("Packet input failed: %d (len:%d)\n", enclen, payload_len);
    }
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_ERRORS);
    if(has_received_seqno && is_using_nack) {
      send_report();
    }
    return;
  }

  if(pinfo.fpmode != SPARROW_ENCAP_FP_MODE_LENOPT) {
    if(verbose_output) {
      printf("Packet input failed: illegal fpmode %d (len:%d)\n", pinfo.fpmode, payload_len);
    }
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_ERRORS);
    if(has_received_seqno && is_using_nack) {
      send_report();
    }
    return;
  }

  if(pinfo.fplen != 4 || pinfo.fp == NULL ||
     (pinfo.fp[1] != SPARROW_ENCAP_FP_LENOPT_OPTION_CRC
      && pinfo.fp[1] != SPARROW_ENCAP_FP_LENOPT_OPTION_SEQNO_CRC)) {
    if(verbose_output) {
      printf("Packet input failed: no CRC in fingerprint (len:%d)\n", payload_len);
    }
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_ERRORS);
    if(has_received_seqno && is_using_nack) {
      send_report();
    }
    return;
  }

  if(pinfo.fpmode == SPARROW_ENCAP_FP_MODE_LENOPT
     && pinfo.fplen == 4 && pinfo.fp
     && pinfo.fp[1] == SPARROW_ENCAP_FP_LENOPT_OPTION_SEQNO_CRC) {
    /* Packet includes sequence number */
    seqno = payload[enclen + 0] << 24;
    seqno |= payload[enclen + 1] << 16;
    seqno |= payload[enclen + 2] << 8;
    seqno |= payload[enclen + 3];
    enclen += 4;

    if(has_received_seqno) {

      if(seqno == last_received_seqno) {
        /* This packet has already been received */
        if(verbose_output) {
          printf("*** Sdup %lu\n", seqno);
        }
        send_report();
        return;
      }

      if(last_received_seqno - seqno < 50) {
        /* Received an older packet - drop it quietly */
        if(verbose_output) {
          printf("*** Sdrop %lu (%lu)\n", seqno, last_received_seqno);
        }
        return;
      }
    } else {
      has_received_seqno = 1;
    }

    last_received_seqno = seqno;
    send_report();
  }

  if(pinfo.payload_type == SPARROW_ENCAP_PAYLOAD_RECEIVE_REPORT) {
    seqno = payload[enclen + 0] << 24;
    seqno |= payload[enclen + 1] << 16;
    seqno |= payload[enclen + 2] << 8;
    seqno |= payload[enclen + 3];

    if(verbose_output) {
      /* printf("report: %lu (%lu)\n", seqno, last_sent_seqno); */
    }

    if(last_packet_len > 0) {
      if(seqno == last_packet_seqno) {
        /* Packet has been received */

      } else if(seqno + 1 == last_packet_seqno) {
        /* Only one packet buffer */
        if(verbose_output) {
          printf("*** R %lu < %lu (%lu)\n", seqno, last_packet_seqno, last_sent_seqno);
        }

        /* Retransmit last packet */
        if(slip_frame_start() == WRITE_STATUS_OK) {
          send_to_slip(last_packet_data, last_packet_len);
          slip_frame_end();
        }
      }

      /* At most one retransmission */
      last_packet_len = 0;
    }

    /* no need to send receive reports further */
    return;
  }

  /* Pass the packet over to the serial radio */
  serial_radio_input(&pinfo, payload, enclen);
}
/*---------------------------------------------------------------------------*/
void
enc_net_send_packet(const uint8_t *ptr, int len)
{
  enc_net_send_packet_payload_type(ptr, len, SPARROW_ENCAP_PAYLOAD_SERIAL);
}

void
enc_net_send_packet_payload_type(const uint8_t *ptr, int len, uint8_t payload_type)
{
  uint8_t buffer[len + 68];
  uint8_t finger[4];
  uint32_t crc_value;
  size_t enc_res;
  int use_seqno = 0;
  sparrow_encap_pdu_info_t pinfo;

  finger[0] = 0;
  finger[1] = SPARROW_ENCAP_FP_LENOPT_OPTION_CRC;
  finger[2] = (len >> 8);
  finger[3] = len & 0xff;

  if(is_using_ack) {

    /* Only use sequence number for serial or TLV data */
    if(payload_type == SPARROW_ENCAP_PAYLOAD_SERIAL || payload_type == SPARROW_ENCAP_PAYLOAD_TLV) {

      /* Only use sequence number if the packet is expected to be small enough to
         be stored for retransmission. */
      if(len + 4 + 4 + 4 < sizeof(last_packet_data)) {
        use_seqno = 1;
        finger[1] = SPARROW_ENCAP_FP_LENOPT_OPTION_SEQNO_CRC;
      }
    }
  }

  pinfo.version = SPARROW_ENCAP_VERSION1;
  pinfo.fp = finger;
  pinfo.iv = NULL;
  pinfo.payload_len = len;
  pinfo.payload_type = payload_type;
  pinfo.fpmode = SPARROW_ENCAP_FP_MODE_LENOPT;
  pinfo.fplen = 4;
  pinfo.ivmode = SPARROW_ENCAP_IVMODE_NONE;
  pinfo.ivlen = 0;

  enc_res = sparrow_encap_write_header(buffer, sizeof(buffer), &pinfo);

  if(! enc_res) {
    printf("*** failed to send encap\n");
    return;
  }

  if(len + enc_res + 4 + (use_seqno ? 4 : 0) > sizeof(buffer)) {
    /* Too large packet */
    printf("Too large packet to send - hdr %u, len %u\n", enc_res, len);
    return;
  }

  if(use_seqno) {
    last_sent_seqno = next_seqno++;
    buffer[enc_res + 0] = (last_sent_seqno >> 24) & 0xff;
    buffer[enc_res + 1] = (last_sent_seqno >> 16) & 0xff;
    buffer[enc_res + 2] = (last_sent_seqno >>  8) & 0xff;
    buffer[enc_res + 3] = last_sent_seqno & 0xff;

    enc_res += 4;
  }

  /* copy the data into the buffer */
  memcpy(buffer + enc_res, ptr, len);

  len += enc_res;

  /* do a crc32 calculation of the whole message */
  crc_value = crc32(buffer, len);

  /* Store the calculated CRC */
  buffer[len++] = (crc_value >> 0L) & 0xff;
  buffer[len++] = (crc_value >> 8L) & 0xff;
  buffer[len++] = (crc_value >> 16L) & 0xff;
  buffer[len++] = (crc_value >> 24L) & 0xff;

  if(use_seqno && len <= sizeof(last_packet_data)) {
    /* Buffer the last packet for retransmissions */
    memcpy(last_packet_data, buffer, len);
    last_packet_len = len;
    last_packet_seqno = last_sent_seqno;

#if TIMED_RESEND
    ctimer_set(&resend_timer, CLOCK_SECOND / 16, resend_callback, NULL);
#endif
  }

  /* out with the whole message */
  if(slip_frame_start() == WRITE_STATUS_OK) {
    if(send_to_slip(buffer, len) == WRITE_STATUS_OK) {
      /* packet successfully sent */
    }

    slip_frame_end();
  }
}
/*---------------------------------------------------------------------------*/
static void
encnet_input(void)
{
  if(border_router_api_version < 3) {
    /* old style */
    int i;
    uip_len = packetbuf_datalen();
    i = packetbuf_copyto(uip_buf);

    if(verbose_output > 1) {
      printf("Slipnet got input of len: %d, copied: %d\n",
	     packetbuf_datalen(), i);

      if(DEBUG) {
        for(i = 0; i < uip_len; i++) {
          printf("%02x", (unsigned char) uip_buf[i]);
          if((i & 15) == 15) {
            printf("\n");
          } else if((i & 7) == 7) {
            printf(" ");
          }
        }
        printf("\n");
      }
    }
    enc_net_send_packet(uip_buf, uip_len);
  } else {
    int pos;
#if !RADIO_802154_WITH_TIMESTAMP
    packetbuf_set_attr(PACKETBUF_ATTR_TIMESTAMP, clock_time() & 0xffff);
#else
    PRINTF("slip-net getting TS from radio %u %u\n",
           packetbuf_attr(PACKETBUF_ATTR_TIMESTAMP),
           (unsigned int)(clock_time() & 0xffff));
#endif
    uip_buf[0] = '!';
    uip_buf[1] = 'S';
    pos = packetutils_serialize_atts(&uip_buf[2], UIP_BUFSIZE - 2);
    uip_len = 2 + pos + packetbuf_datalen();

    if(pos < 0) {
      if(verbose_output > 1) {
        PRINTF("slip-net: failed to serialize atts\n");
      }
    } else if(uip_len > UIP_BUFSIZE) {
      if(verbose_output > 1) {
        PRINTF("slip-net: too large packet: %d\n", uip_len);
      }
    } else {
      packetbuf_copyto(&uip_buf[pos + 2]);

      if(verbose_output > 1) {
        printf("Slipnet got input of len: %d, total out: %d\n",
               packetbuf_datalen(), uip_len);
      }

      enc_net_send_packet(uip_buf, uip_len);
    }
  }

  uip_clear_buf();
}
/*---------------------------------------------------------------------------*/
const struct network_driver encnet_driver = {
  "encnet",
  encnet_init,
  encnet_input
};
/*---------------------------------------------------------------------------*/
