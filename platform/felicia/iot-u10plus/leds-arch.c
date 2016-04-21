/*
 * Copyright (c) 2015, Yanzi Networks AB.
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
 */

#include "contiki.h"
#include "dev/leds.h"
#include "dev/gpio.h"

#define USER_GREEN_PORT_BASE     GPIO_A_BASE
#define USER_GREEN_PIN_MASK      GPIO_PIN_MASK(4)

#define USER_YELLOW_PORT_BASE    GPIO_A_BASE
#define USER_YELLOW_PIN_MASK     GPIO_PIN_MASK(5)

#define SYSTEM_YELLOW_PORT_BASE  GPIO_C_BASE
#define SYSTEM_YELLOW_PIN_MASK   GPIO_PIN_MASK(0)

#define SYSTEM_GREEN_PORT_BASE   GPIO_C_BASE
#define SYSTEM_GREEN_PIN_MASK    GPIO_PIN_MASK(1)
/*---------------------------------------------------------------------------*/
void
leds_arch_init(void)
{
  GPIO_SET_OUTPUT(USER_GREEN_PORT_BASE, USER_GREEN_PIN_MASK);
  GPIO_SET_PIN(USER_GREEN_PORT_BASE, USER_GREEN_PIN_MASK);

  GPIO_SET_OUTPUT(USER_YELLOW_PORT_BASE, USER_YELLOW_PIN_MASK);
  GPIO_SET_PIN(USER_YELLOW_PORT_BASE, USER_YELLOW_PIN_MASK);

  GPIO_SET_OUTPUT(SYSTEM_YELLOW_PORT_BASE, SYSTEM_YELLOW_PIN_MASK);
  GPIO_SET_PIN(SYSTEM_YELLOW_PORT_BASE, SYSTEM_YELLOW_PIN_MASK);

  GPIO_SET_OUTPUT(SYSTEM_GREEN_PORT_BASE, SYSTEM_GREEN_PIN_MASK);
  GPIO_SET_PIN(SYSTEM_GREEN_PORT_BASE, SYSTEM_GREEN_PIN_MASK);
}
/*---------------------------------------------------------------------------*/
unsigned char
leds_arch_get(void)
{
  return (GPIO_READ_PIN(USER_YELLOW_PORT_BASE, USER_YELLOW_PIN_MASK) ? 0 : LEDS_YELLOW) +
    (GPIO_READ_PIN(USER_GREEN_PORT_BASE, USER_GREEN_PIN_MASK) ? 0 : LEDS_GREEN) +
    (GPIO_READ_PIN(SYSTEM_YELLOW_PORT_BASE, SYSTEM_YELLOW_PIN_MASK) ? 0 : LEDS_SYSTEM_YELLOW) +
    (GPIO_READ_PIN(SYSTEM_GREEN_PORT_BASE, SYSTEM_GREEN_PIN_MASK) ? 0 : LEDS_SYSTEM_GREEN);
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(unsigned char leds)
{
  if(leds & LEDS_YELLOW) {
    GPIO_CLR_PIN(USER_YELLOW_PORT_BASE, USER_YELLOW_PIN_MASK);
  } else {
    GPIO_SET_PIN(USER_YELLOW_PORT_BASE, USER_YELLOW_PIN_MASK);
  }
  if(leds & LEDS_GREEN) {
    GPIO_CLR_PIN(USER_GREEN_PORT_BASE, USER_GREEN_PIN_MASK);
  } else {
    GPIO_SET_PIN(USER_GREEN_PORT_BASE, USER_GREEN_PIN_MASK);
  }

  if(leds & LEDS_SYSTEM_YELLOW) {
    GPIO_CLR_PIN(SYSTEM_YELLOW_PORT_BASE, SYSTEM_YELLOW_PIN_MASK);
  } else {
    GPIO_SET_PIN(SYSTEM_YELLOW_PORT_BASE, SYSTEM_YELLOW_PIN_MASK);
  }
  if(leds & LEDS_SYSTEM_GREEN) {
    GPIO_CLR_PIN(SYSTEM_GREEN_PORT_BASE, SYSTEM_GREEN_PIN_MASK);
  } else {
    GPIO_SET_PIN(SYSTEM_GREEN_PORT_BASE, SYSTEM_GREEN_PIN_MASK);
  }
}
/*---------------------------------------------------------------------------*/
