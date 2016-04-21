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
/*---------------------------------------------------------------------------*/
static int
user_button_value(int type)
{
  return (GPIO_READ_PIN(USER_BUTTON_PORT_BASE,
                        USER_BUTTON_PIN_MASK) == 0) ||
    !timer_expired(&debouncetimer);
}
/*---------------------------------------------------------------------------*/
static int
user_button_status(int type)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief PIN initializer for platform (user) button
 * \param port_base GPIO port's register offset
 * \param pin_mask Pin mask corresponding to the button's pin
 */
static void
config(uint32_t port_base, uint32_t pin_mask)
{
  /* Software controlled */
  GPIO_SOFTWARE_CONTROL(port_base, pin_mask);

  /* Set pin to input */
  GPIO_SET_INPUT(port_base, pin_mask);

  /* Enable edge detection */
  GPIO_DETECT_EDGE(port_base, pin_mask);

  /* Single edge */
  GPIO_TRIGGER_SINGLE_EDGE(port_base, pin_mask);

  /* Trigger interrupt on Falling edge */
  GPIO_DETECT_FALLING(port_base, pin_mask);

  GPIO_ENABLE_INTERRUPT(port_base, pin_mask);
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
user_button_irq_handler(uint8_t port, uint8_t pin)
{
  if(!timer_expired(&debouncetimer)) {
    return;
  }

  timer_set(&debouncetimer, CLOCK_SECOND / 8);
  sensors_changed(&button_sensor);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialize configuration for the user button.
 *
 * \param type ignored
 * \param value ignored
 * \return ignored
 */
static int
config_user_button(int type, int value)
{
  config(USER_BUTTON_PORT_BASE, USER_BUTTON_PIN_MASK);

  ioc_set_over(USER_BUTTON_PORT, USER_BUTTON_PIN, IOC_OVERRIDE_PUE);

  timer_set(&debouncetimer, 0);

  nvic_interrupt_enable(USER_BUTTON_VECTOR);

  gpio_register_callback(user_button_irq_handler,
    USER_BUTTON_PORT, USER_BUTTON_PIN);
  return 1;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(button_sensor, BUTTON_SENSOR, user_button_value, config_user_button, user_button_status);
