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
 *         Implementation of the extended leds API
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "dev/leds-ext.h"
#include "sys/cc.h"
/*---------------------------------------------------------------------------*/
unsigned
leds_get_state(const leds_t *led)
{
  if(led != NULL && led->get_state != NULL) {
    return led->get_state(led);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
leds_set_state(const leds_t *led, unsigned state)
{
  if(led != NULL && led->set_state != NULL) {
    return led->set_state(led, state);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
leds_get_state_count(const leds_t *led)
{
  if(led == NULL) {
    return 0;
  }
  if(led->get_state_count != NULL) {
    return led->get_state_count(led);
  }
  /* Default is on or off */
  return 2;
}
/*---------------------------------------------------------------------------*/
uint32_t
leds_get_rgb(const leds_t *led)
{
  if(led != NULL && led->get_rgb != NULL) {
    return led->get_rgb(led);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
leds_set_rgb(const leds_t *led, uint32_t rgb)
{
  if(led != NULL && led->set_rgb != NULL) {
    return led->set_rgb(led, rgb);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
