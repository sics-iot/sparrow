/* -*- C -*- */
/*
 * Copyright (c) 2005-2015, Swedish Institute of Computer Science
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
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "sparrow.h"
#include "net/ip/uip.h"
#include "dev/slip.h"
#include "serial-radio-stats.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

#ifndef ATOMIC
#ifndef INTERRUPTS_DISABLE
#error Please add support for disabling interrupts
#endif /* INTERRUPTS_DISABLE */
#define ATOMIC(code) do {                  \
    INTERRUPTS_DISABLE();                  \
    code;                                  \
    INTERRUPTS_ENABLE();                   \
  } while(0);
#endif /* ATOMIC */

PROCESS(slip_process, "SLIP driver");

#if 1
#define SLIP_STATISTICS(statement)
#else
uint16_t slip_drop_bytes, slip_overflow, slip_error_drop;
/* No used in this file */
uint16_t slip_rubbish, slip_twopackets, slip_ip_drop;
unsigned long slip_received, slip_frames;
#define SLIP_STATISTICS(statement) statement
#endif

/* Must be at least as large as UIP_BUFSIZE! */
#define RX_BUFSIZE (UIP_BUFSIZE > 1500 ? UIP_BUFSIZE : 1500)

/*
 * Variables begin and end manage the buffer space in a cyclic
 * fashion. The first used byte is at begin and end is one byte past
 * the last. I.e. [begin, end) is the actively used space.
 */

static volatile uint16_t begin, end, end_counter;
static uint8_t rxbuf[RX_BUFSIZE];
static uint8_t is_dropping = 0;

static void (* input_callback)(void) = NULL;

int slip_writeb(unsigned char c);
/*---------------------------------------------------------------------------*/
void
slip_set_input_callback(void (*c)(void))
{
  input_callback = c;
}
/*---------------------------------------------------------------------------*/
uint8_t
slip_write(const void *_ptr, int len)
{
  const uint8_t *ptr = _ptr;
  uint16_t i;
  uint8_t c;

  slip_writeb(SLIP_END);

  for(i = 0; i < len; ++i) {
    c = *ptr++;
    if(c == SLIP_END) {
      slip_writeb(SLIP_ESC);
      c = SLIP_ESC_END;
    } else if(c == SLIP_ESC) {
      slip_writeb(SLIP_ESC);
      c = SLIP_ESC_ESC;
    }
    slip_writeb(c);
  }
  slip_writeb(SLIP_END);

  return len;
}
/*---------------------------------------------------------------------------*/
static void
rxbuf_init(void)
{
  begin = end = end_counter = 0;
  is_dropping = 0;
}
/*---------------------------------------------------------------------------*/
/* Upper half does the polling. */
static uint16_t
slip_poll_handler(uint8_t *outbuf, uint16_t blen)
{
  uint16_t len;
  uint16_t pos;
  uint8_t c;
  uint8_t state;

  if(end_counter == 0) {
    return 0;
  }

  for(len = 0, pos = begin, state = c = 0;;) {
    c = rxbuf[pos++];
    if(pos == RX_BUFSIZE) {
      pos = 0;
    }

    if(c == SLIP_END) {
      /* End of packet */
      break;
    }

    if(len >= blen) {
      break;
    }

    switch(c) {
    case SLIP_ESC:
      state = SLIP_ESC;
      break;
    case SLIP_ESC_END:
      if(state == SLIP_ESC) {
        outbuf[len++] = SLIP_END;
        state = 0;
      } else {
        outbuf[len++] = SLIP_ESC_END;
      }
      break;
    case SLIP_ESC_ESC:
      if(state == SLIP_ESC) {
        outbuf[len++] = SLIP_ESC;
        state = 0;
      } else {
        outbuf[len++] = SLIP_ESC_ESC;
      }
      break;
    default:
      outbuf[len++] = c;
      state = 0;
      break;
    }
  }

  /* Update counters */
  if(c == SLIP_END) {
    ATOMIC(begin = pos;
           end_counter--;
           )
  } else {
    /* Something went wrong, no SLIP_END found, drop everything */
    ATOMIC(rxbuf_init();
           is_dropping = 1;
           )
    SLIP_STATISTICS(slip_error_drop++);
    SERIAL_RADIO_STATS_DEBUG_INC(SERIAL_RADIO_STATS_DEBUG_SLIP_ERRORS);
    len = 0;
    PRINTF("SLIP: *** out of sync!\n");
  }

  if(end_counter > 0) {
    /* One more packet is buffered, need to be polled again! */
    process_poll(&slip_process);
  }

  return len;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(slip_process, ev, data)
{
  PROCESS_BEGIN();

  rxbuf_init();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    uip_len = slip_poll_handler(uip_buf, UIP_BUFSIZE);
    PRINTF("SLIP: recv %lu bytes %u frames RECV: %u\n",
           SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_RECV), end_counter, uip_len);
    if(uip_len > 0) {
      if(input_callback) {
        input_callback();
      }
    }
    uip_clear_buf();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
int
slip_input_byte(unsigned char c)
{
  static int in_frame = 0;
  uint16_t next;

  SLIP_STATISTICS(slip_received++);
  SERIAL_RADIO_STATS_DEBUG_INC(SERIAL_RADIO_STATS_DEBUG_SLIP_RECV);

  if(is_dropping) {
    if(c != SLIP_END) {
      SLIP_STATISTICS(slip_drop_bytes++);
      SERIAL_RADIO_STATS_DEBUG_INC(SERIAL_RADIO_STATS_DEBUG_SLIP_DROPPED);
    } else {
      is_dropping = 0;
      in_frame = 0;
    }
    return 0;
  }

  if(!in_frame && c == SLIP_END) {
    /* Ignore slip end when not receiving frame */
    return 0;
  }

  next = end + 1;
  if(next >= RX_BUFSIZE) {
    next = 0;
  }
  if(next == begin) {
    /* Buffer is full */
    is_dropping = 1;
    SLIP_STATISTICS(slip_overflow++);
    SERIAL_RADIO_STATS_DEBUG_INC(SERIAL_RADIO_STATS_DEBUG_SLIP_OVERFLOWS);
    return 0;
  }
  rxbuf[end] = c;
  end = next;
  in_frame = 1;

  if(c == SLIP_END) {
    in_frame = 0;
    end_counter++;
    SLIP_STATISTICS(slip_frames++);
    SERIAL_RADIO_STATS_DEBUG_INC(SERIAL_RADIO_STATS_DEBUG_SLIP_FRAMES);
    process_poll(&slip_process);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
