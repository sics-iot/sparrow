/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
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
 *         An implementation of the extended leds API for platform Zoul
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "dev/leds.h"
#include "dev/leds-ext.h"
#include "sys/cc.h"

#if PLATFORM_HAS_LEDS_EXT

static const uint8_t states[] = {
  0, LEDS_GREEN, LEDS_BLUE, LEDS_RED,
  LEDS_LIGHT_BLUE, LEDS_YELLOW, LEDS_PURPLE,
  LEDS_WHITE
};
#define STATE_COUNT (sizeof(states) / sizeof(uint8_t))
/*---------------------------------------------------------------------------*/
static unsigned
get_state(const leds_t *led)
{
  unsigned char current;
  int i;
  /* The Zoul ext led is implemented using Contiki leds API */
  current = leds_get();
  if(current != 0) {
    for(i = 1; i < STATE_COUNT; i++) {
      if(current == states[i]) {
        return i;
      }
    }
  }
  return LEDS_STATE_OFF;
}
/*---------------------------------------------------------------------------*/
static int
set_state(const leds_t *led, unsigned state)
{
  if(state < STATE_COUNT) {
    leds_set(states[state]);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
get_state_count(const leds_t *led)
{
  return STATE_COUNT;
}
/*---------------------------------------------------------------------------*/
void
leds_ext_arch_init(void)
{
}
/*---------------------------------------------------------------------------*/
const leds_t system_led = {
  .get_state = get_state,
  .set_state = set_state,
  .get_state_count = get_state_count,
};
/*---------------------------------------------------------------------------*/
LEDS(&system_led);
/*---------------------------------------------------------------------------*/
#endif /* PLATFORM_HAS_LEDS_EXT */
