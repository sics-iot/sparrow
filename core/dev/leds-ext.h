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
 *         An extended leds API - to be merged with the leds.h
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef LEDS_EXT_H_
#define LEDS_EXT_H_

#include "contiki-conf.h"
#include "dev/leds.h"

#define LEDS_STATE_OFF  0

#ifdef LEDS_CONF_STATE_TYPE
#define LEDS_STATE_TYPE LEDS_CONF_STATE_TYPE
#endif /* LEDS_CONF_STATE_TYPE */

typedef struct leds leds_t;

struct leds {
#ifdef LEDS_STATE_TYPE
  LEDS_STATE_TYPE data;
#endif /* LEDS_STATE_TYPE */

  unsigned (* get_state)(const leds_t *led);
  int (* set_state)(const leds_t *led, unsigned color_index);
  int (* get_state_count)(const leds_t *led);

  uint32_t (* get_rgb)(const leds_t *led);
  int (* set_rgb)(const leds_t *led, uint32_t rgb);

  uint8_t (* get_level)(const leds_t *led);
  void (* set_level)(const leds_t *led, uint8_t level);
};

extern const leds_t *leds[];
extern const unsigned leds_count;

#define LEDS(...)                                                       \
  const leds_t *leds[] = {__VA_ARGS__, NULL};                           \
  const unsigned leds_count = sizeof(leds) / sizeof(const leds_t *) - 1

/*---------------------------------------------------------------------------*/
unsigned leds_get_state(const leds_t *led);
int leds_set_state(const leds_t *led, unsigned color_index);
int leds_get_state_count(const leds_t *led);
uint32_t leds_get_rgb(const leds_t *led);
int leds_set_rgb(const leds_t *led, uint32_t rgb);

void leds_ext_arch_init(void);

static inline void
leds_ext_init(void)
{
  leds_ext_arch_init();
}

#endif /* LEDS_EXT_H_ */
