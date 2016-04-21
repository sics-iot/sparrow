/*
 * Copyright (c) 2014-2016, SICS, Swedish ICT AB.
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
 *         Frontpanel for the serial radio
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef RADIO_FRONTPANEL_H_
#define RADIO_FRONTPANEL_H_

#include "contiki-conf.h"

uint32_t radio_frontpanel_get_info(void);
void radio_frontpanel_set_info(uint32_t info);

uint32_t radio_frontpanel_get_error_code(void);
void radio_frontpanel_set_error_code(uint32_t info);

void radio_frontpanel_show_error_code();

uint32_t radio_frontpanel_get_watchdog(void);
void radio_frontpanel_set_watchdog(uint32_t watchdog);

uint32_t radio_frontpanel_get_supply_status(void);
uint32_t radio_frontpanel_get_battery_supply_time(void);
uint32_t radio_frontpanel_get_supply_voltage(void);

uint16_t radio_frontpanel_get_charger_pulse_count(void);

uint32_t radio_frontpanel_get_fan_speed(void);
void     radio_frontpanel_set_fan_speed(uint32_t speed);

void radio_frontpanel_init(void);

#endif /* RADIO_FRONTPANEL_H_ */
