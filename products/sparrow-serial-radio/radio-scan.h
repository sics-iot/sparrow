/*
 * Copyright (c) 2013, Swedish Institute of Computer Science.
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
 *         Radio RSSI scanner
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef RADIO_SCAN_H_
#define RADIO_SCAN_H_

#include "sys/clock.h"

typedef enum {
  RADIO_SCAN_MODE_NORMAL           = 0,
  RADIO_SCAN_MODE_RSSI             = 1,
  RADIO_SCAN_MODE_RSSI_CHANNEL     = 2,
  RADIO_SCAN_MODE_RSSI_CHANNEL_AGG = 3
} radio_scan_mode_t;

typedef struct {
  int8_t avg;
  int8_t min;
  int8_t max;
  uint8_t channel;
  uint8_t count[4];
} radio_scan_energy_entry_t;

radio_scan_mode_t radio_scan_get_mode(void);
void radio_scan_set_mode(radio_scan_mode_t mode);

void radio_scan_set_channel(uint8_t channel);

clock_time_t radio_scan_get_duration(void);
void radio_scan_set_duration(clock_time_t scan_time);

void radio_scan_start(void);
void radio_scan_stop(void);

#endif /* RADIO_SCAN_H_ */
