/*
 * Copyright (c) 2013-2016, Swedish Institute of Computer Science.
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
 */

#include "contiki.h"
#include "radio-scan.h"
#include "serial-radio.h"
#include "cmd.h"
#include "net/netstack.h"
#include <stdio.h>

PROCESS(scan_process, "Radio scan process");

struct radio_scan {
  int32_t rssi_sum;
  int8_t rssi_max;
  int8_t rssi_min;
  uint16_t count;
};

#define DELAY_BETWEEN_SCANS 5

static radio_scan_mode_t scan_mode = RADIO_SCAN_MODE_NORMAL;
static clock_time_t scan_duration = CLOCK_SECOND / 2;
static uint16_t scan_channels = 0;
static uint8_t active_channel;
static uint8_t is_scanning;
/*---------------------------------------------------------------------------*/
static void
add_energy_scan(uint8_t channel, int32_t sum, int8_t min, int8_t max, uint32_t count)
{
  /* TODO store energy scan data */
}
/*---------------------------------------------------------------------------*/
radio_scan_mode_t
radio_scan_get_mode(void)
{
  return scan_mode;
}
/*---------------------------------------------------------------------------*/
void
radio_scan_set_mode(radio_scan_mode_t mode)
{
  if(scan_mode != mode) {
    scan_mode = mode;
    if(is_scanning) {
      /* Restart the scanning process */
      radio_scan_start();
    }
  }
}
/*---------------------------------------------------------------------------*/
void
radio_scan_set_channel(uint8_t channel)
{
  if(is_scanning && scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL) {
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel);
    active_channel = channel - 11;
  }
}
/*---------------------------------------------------------------------------*/
uint16_t
radio_scan_get_active_channels(void)
{
  return scan_channels;
}
/*---------------------------------------------------------------------------*/
void
radio_scan_set_active_channels(uint16_t channels)
{
  scan_channels = channels;
}
/*---------------------------------------------------------------------------*/
clock_time_t
radio_scan_get_duration(void)
{
  return scan_duration;
}
/*---------------------------------------------------------------------------*/
void
radio_scan_set_duration(clock_time_t duration)
{
  scan_duration = duration;
}
/*---------------------------------------------------------------------------*/
void
radio_scan_start(void)
{
  radio_value_t channel;

  /* Stop any running scan */
  if(is_scanning) {
    radio_scan_stop();
  }

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &channel) == RADIO_RESULT_OK
     && channel > 10) {
    is_scanning = 1;
    active_channel = channel - 11;
    process_start(&scan_process, NULL);
  } else {
    /* Failed to get channel from radio (should never happen). */
    printf("radio scan: failed to start scan\n");
  }
}
/*---------------------------------------------------------------------------*/
void
radio_scan_stop(void)
{
  is_scanning = 0;
  process_exit(&scan_process);
}
/*---------------------------------------------------------------------------*/
/* Fast scan of selected channels */
static void
scan_all_channels(void)
{
  radio_value_t value;
  int pos = 3;
  int8_t rssi;
  radio_value_t max;
  radio_value_t min;

  if(NETSTACK_RADIO.get_value(RADIO_CONST_SUBCHANNEL_MAX, &max) == RADIO_RESULT_OK &&
     NETSTACK_RADIO.get_value(RADIO_CONST_SUBCHANNEL_MIN, &min) == RADIO_RESULT_OK &&
     min <= max && max - min < 512) {
    uint8_t buf[4 + max - min + 1];
    for(active_channel = min; active_channel <= max; active_channel++) {
      NETSTACK_RADIO.set_value(RADIO_PARAM_SUBCHANNEL, active_channel);
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value) == RADIO_RESULT_OK) {
        rssi = value;
      } else {
        rssi = ~0;
      }
      buf[pos++] = rssi;
    }
    buf[0] = '!';
    buf[1] = 'r';
    buf[2] = 'A';
    cmd_send(buf, pos);
    return;
  }

  if(NETSTACK_RADIO.get_value(RADIO_CONST_CHANNEL_MAX, &max) == RADIO_RESULT_OK &&
     NETSTACK_RADIO.get_value(RADIO_CONST_CHANNEL_MIN, &min) == RADIO_RESULT_OK &&
     min <= max && max - min < 512)
  {
    uint8_t buf[4 + max - min + 1];
    for(active_channel = min; active_channel <= max; active_channel++) {
      NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, active_channel);
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value) == RADIO_RESULT_OK) {
        rssi = value;
      } else {
        rssi = ~0;
      }
      buf[pos++] = rssi;
    }
    buf[0] = '!';
    buf[1] = 'r';
    buf[2] = 'A';
    cmd_send(buf, pos);
    return;
  }

  /* Assume standard 802.15.4 16 channels */
  if(1) {
    uint8_t buf[4 + 16];
    for(active_channel = 0; active_channel < 16; active_channel++) {
      NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, 11 + active_channel);
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value) == RADIO_RESULT_OK) {
        rssi = value;
      } else {
        rssi = ~0;
      }
      buf[pos++] = rssi;
    }
    buf[0] = '!';
    buf[1] = 'r';
    buf[2] = 'A';
    cmd_send(buf, pos);
  }
}
/*---------------------------------------------------------------------------*/
/* Fast scan of single channel */
#define RSSI_CHANNEL_COUNT 32
static void
scan_single_channel(void)
{
  uint8_t buf[2 + 2 + RSSI_CHANNEL_COUNT];
  radio_value_t value;
  int pos, i;
  int8_t rssi;

  pos = 0;
  buf[pos++] = '!';
  buf[pos++] = 'r';
  buf[pos++] = 'C';
  for(i = 0; i < RSSI_CHANNEL_COUNT; i++) {
    if(NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value) == RADIO_RESULT_OK) {
      rssi = value;
    } else {
      rssi = ~0;
    }
    buf[pos++] = rssi;
  }
  cmd_send(buf, pos);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(scan_process, ev, data)
{
  static struct radio_scan scan_info;
  static struct etimer scan_timer;
  static struct etimer scan_delay_timer;

  PROCESS_BEGIN();

  while(is_scanning) {
    etimer_set(&scan_timer, DELAY_BETWEEN_SCANS);
    PROCESS_WAIT_UNTIL(etimer_expired(&scan_timer));

    if(scan_mode == RADIO_SCAN_MODE_NORMAL) {
      int8_t rssi;
      radio_value_t value;

      /* Normal scan of selected channels */
      for(active_channel = 0; active_channel < 16; active_channel++) {
        if(scan_channels && !(scan_channels & (1 << active_channel))) {
          /* Skip this channel */
          continue;
        }
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, 11 + active_channel);

        PROCESS_PAUSE();

        etimer_set(&scan_timer, scan_duration);
        if(NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value) == RADIO_RESULT_OK) {
          rssi = (int8_t)(value & 0xff);
        } else {
          rssi = ~0;
        }
        scan_info.rssi_max = rssi;
        scan_info.rssi_min = rssi;
        scan_info.rssi_sum = rssi;
        scan_info.count = 1;

        while(!etimer_expired(&scan_timer)) {
          etimer_set(&scan_delay_timer, DELAY_BETWEEN_SCANS);
          PROCESS_WAIT_UNTIL(etimer_expired(&scan_delay_timer));

          if(NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value) == RADIO_RESULT_OK) {
            rssi = (int8_t)(value & 0xff);
          } else {
            rssi = ~0;
          }
          if(rssi > scan_info.rssi_max) {
            scan_info.rssi_max = rssi;
          }
          if(rssi < scan_info.rssi_min) {
            scan_info.rssi_min = rssi;
          }
          scan_info.rssi_sum += rssi;
          scan_info.count++;
        }

        add_energy_scan(11 + active_channel,
                        scan_info.rssi_sum,
                        scan_info.rssi_max,
                        scan_info.rssi_min,
                        scan_info.count);
        /*
        buf[0] = '!';
        buf[1] = 'e';
        buf[2] = 11 + active_channel;
        buf[3] = scan_info.count >> 8;
        buf[4] = scan_info.count & 0xff;
        buf[5] = avg;
        buf[6] = scan_info.rssi_max;
        buf[7] = scan_info.rssi_min;
        cmd_send(buf, 8);
        */
      }
    } else if(scan_mode == RADIO_SCAN_MODE_RSSI) {
      scan_all_channels();

      etimer_set(&scan_delay_timer, 1);
      PROCESS_WAIT_UNTIL(etimer_expired(&scan_delay_timer));

    } else if(scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL) {
      scan_single_channel();

      etimer_set(&scan_delay_timer, 1);
      PROCESS_WAIT_UNTIL(etimer_expired(&scan_delay_timer));
    } else {
      printf("radio scan: unhandled scan mode: %d\n", scan_mode);
      is_scanning = 0;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
