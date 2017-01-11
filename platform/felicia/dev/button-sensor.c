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
#include "dev/nvic.h"
#include "dev/ioc.h"
#include "dev/gpio.h"
#include "dev/button-sensor.h"
#include "sys/timer.h"

#include <stdint.h>
#include <string.h>

#define USER_BUTTON_PORT_BASE  GPIO_PORT_TO_BASE(USER_BUTTON_PORT)
#define USER_BUTTON_PIN_MASK   GPIO_PIN_MASK(USER_BUTTON_PIN)

static struct timer debouncetimer;
static uint8_t current_status = 0;
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  return (GPIO_READ_PIN(USER_BUTTON_PORT_BASE, USER_BUTTON_PIN_MASK) == 0) ||
    !timer_expired(&debouncetimer);
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  if(type == SENSORS_ACTIVE || type == SENSORS_READY) {
    return (current_status & 2) != 0;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Interrupt handler for the user button.
 *
 * \param port The port number that generated the interrupt
 * \param pin The pin number that generated the interrupt.
 *
 * This function is called inside interrupt context.
 */
static void
button_irq_handler(uint8_t port, uint8_t pin)
{
  if(!timer_expired(&debouncetimer)) {
    return;
  }

  timer_set(&debouncetimer, CLOCK_SECOND / 8);
  sensors_changed(&button_sensor);
}
/*---------------------------------------------------------------------------*/
static int
config(int type, int value)
{
  if(current_status == 0) {
    /* Initialize hardware */

    GPIO_SOFTWARE_CONTROL(USER_BUTTON_PORT_BASE, USER_BUTTON_PIN_MASK);

    GPIO_SET_INPUT(USER_BUTTON_PORT_BASE, USER_BUTTON_PIN_MASK);

    /* Enable edge detection, on falling edge */
    GPIO_DETECT_EDGE(USER_BUTTON_PORT_BASE, USER_BUTTON_PIN_MASK);
    GPIO_TRIGGER_SINGLE_EDGE(USER_BUTTON_PORT_BASE, USER_BUTTON_PIN_MASK);
    GPIO_DETECT_FALLING(USER_BUTTON_PORT_BASE, USER_BUTTON_PIN_MASK);

    ioc_set_over(USER_BUTTON_PORT, USER_BUTTON_PIN, IOC_OVERRIDE_PUE);

    gpio_register_callback(button_irq_handler,
                           USER_BUTTON_PORT, USER_BUTTON_PIN);

    current_status = 1;
  }

  if(type == SENSORS_ACTIVE) {
    if(value) {
      timer_set(&debouncetimer, 0);
      GPIO_ENABLE_INTERRUPT(USER_BUTTON_PORT_BASE, USER_BUTTON_PIN_MASK);
      NVIC_EnableIRQ(USER_BUTTON_VECTOR);
      current_status |= 2;
    } else {
      GPIO_DISABLE_INTERRUPT(USER_BUTTON_PORT_BASE, USER_BUTTON_PIN_MASK);
      NVIC_DisableIRQ(USER_BUTTON_VECTOR);
      current_status &= ~2;
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(button_sensor, BUTTON_SENSOR, value, config, status);
/*---------------------------------------------------------------------------*/
