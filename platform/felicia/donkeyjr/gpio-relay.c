/*
 * Copyright (c) 2017, SICS, Swedish ICT AB.
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
 *         API to control GPIO optic relay
 * \author
 *         Niclas Finne <nfi@sics.se>
 */

#include "dev/gpio-relay.h"
#include "dev/gpio.h"
#include "dev/ioc.h"

/* DonkeyJR has 4 relays */
#define GPIO_RELAY_COUNT 4

typedef struct {
  uint8_t port;
  uint8_t pin;
} reg_config_t;

static const reg_config_t regs[GPIO_RELAY_COUNT] = {
  { GPIO_RELAY_1_PORT, GPIO_RELAY_1_PIN },
  { GPIO_RELAY_2_PORT, GPIO_RELAY_2_PIN },
  { GPIO_RELAY_3_PORT, GPIO_RELAY_3_PIN },
  { GPIO_RELAY_4_PORT, GPIO_RELAY_4_PIN }
};
/*---------------------------------------------------------------------------*/
int
gpio_relay_get(int index)
{
  if(index < 0 || index >= GPIO_RELAY_COUNT) {
    return 0;
  }
  return GPIO_READ_PIN(GPIO_PORT_TO_BASE(regs[index].port),
                       GPIO_PIN_MASK(regs[index].pin)) != 0;
}
/*---------------------------------------------------------------------------*/
int
gpio_relay_set(int index, int onoroff)
{
  if(index < 0 || index >= GPIO_RELAY_COUNT) {
    return 0;
  }
  if(onoroff) {
    GPIO_CLR_PIN(GPIO_PORT_TO_BASE(regs[index].port),
                 GPIO_PIN_MASK(regs[index].pin));
  } else {
    GPIO_SET_PIN(GPIO_PORT_TO_BASE(regs[index].port),
                 GPIO_PIN_MASK(regs[index].pin));
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
gpio_relay_count(void)
{
  return GPIO_RELAY_COUNT;
}
/*---------------------------------------------------------------------------*/
void
gpio_relay_init(void)
{
  int i;
  const reg_config_t *r;
  for(i = 0; i < GPIO_RELAY_COUNT; i++) {
    r = &regs[i];
    ioc_set_over(r->port, r->pin, IOC_OVERRIDE_OE);
    GPIO_SOFTWARE_CONTROL(GPIO_PORT_TO_BASE(r->port),
                          GPIO_PIN_MASK(r->pin));

#if GPIO_RELAY_DEFAULT_ON
    GPIO_CLR_PIN(GPIO_PORT_TO_BASE(r->port),
                 GPIO_PIN_MASK(r->pin));
#else /* GPIO_RELAY_DEFAULT_ON */
    GPIO_SET_PIN(GPIO_PORT_TO_BASE(r->port),
                 GPIO_PIN_MASK(r->pin));
#endif /* GPIO_RELAY_DEFAULT_ON */

    GPIO_SET_OUTPUT(GPIO_PORT_TO_BASE(r->port),
                    GPIO_PIN_MASK(r->pin));
  }
}
/*---------------------------------------------------------------------------*/
