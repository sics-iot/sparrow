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
 */

#include "contiki.h"
#include "radio-scan.h"
#include "serial-radio.h"
#include "cmd.h"
#include "net/netstack.h"
#ifdef PLATFORM_HAS_CC2538
#include "dev/cc2538-rf.h"
#include "reg.h"
#include "dev/ioc.h"
#include "dev/sys-ctrl.h"
#endif /* PLATFORM_HAS_CC2538 */
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
static uint8_t is_scanning;
static uint8_t is_timer_running;


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
  if(is_scanning && (scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL ||
                     scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL_AGG)) {
    /* Stop the scan before changing channel to ensure radio is not used */
    radio_scan_stop();
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel);
    radio_scan_start();
  }
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

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &channel) == RADIO_RESULT_OK) {
    is_scanning = 1;
    process_start(&scan_process, NULL);
  } else {
    /* Failed to get channel from radio (should never happen). */
    printf("radio scan: failed to start scan\n");
  }
}
/*---------------------------------------------------------------------------*/
static void deinit_timer(void);

void
radio_scan_stop(void)
{
  if(is_timer_running) {
    deinit_timer();
  }

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
  uint8_t active_channel;

#ifdef PLATFORM_HAS_CC2538

  uint8_t buf[4 + CC2538_RF_FREQ_MAX - CC2538_RF_FREQ_MIN + 1];
  for(active_channel = CC2538_RF_FREQ_MIN; active_channel <= CC2538_RF_FREQ_MAX; active_channel++) {
    cc2538_rf_set_central_freq(active_channel);
    if(NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value) == RADIO_RESULT_OK) {
      rssi = value;
    } else {
      rssi = ~0;
    }
    buf[pos++] = rssi;
  }

#else /* PLATFORM_HAS_CC2538 */

  uint8_t buf[4 + 16];
  for(active_channel = 11; active_channel <= 26; active_channel++) {
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, active_channel);
    if(NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value) == RADIO_RESULT_OK) {
      rssi = value;
    } else {
      rssi = ~0;
    }
    buf[pos++] = rssi;
  }

#endif /* PLATFORM_HAS_CC2538 */

  buf[0] = '!';
  buf[1] = 'r';
  buf[2] = 'A';
  cmd_send(buf, pos);
}
/*---------------------------------------------------------------------------*/
/* Fast scan of single channel */
#define RSSI_CHANNEL_COUNT 32

static uint8_t buf_0[2 + 2 + 2 + RSSI_CHANNEL_COUNT];
static uint8_t buf_1[2 + 2 + 2 + RSSI_CHANNEL_COUNT];

static uint8_t pos = 5;
static uint8_t *buf = buf_0;
static uint8_t *ready_buf = NULL;

static uint32_t timer2a;

/* Experimental RSSI-level sendability packet-analysis - buckets */
/*
 *  -81 - -85  dbm
 *  -86 - -90  dbm
 *  -91 - -95  dbm
 *  -96 - -100 dbm
 * -101 - -105 dbm
 * -106 - ...  dbm
 */

#define INIT_RSSI        -81
#define BUCKET_WIDTH      5
#define BUCKET_COUNT      6
static uint32_t last_sample_time[BUCKET_COUNT];
static uint16_t packet_count[BUCKET_COUNT];


/* 1000 * 8-bit value => 32 bits should be ok */
#define COUNT 10000
static int32_t average;
static int32_t variance;

void
gptimer2a_isr(void)
{
  int8_t rssi;
  radio_value_t value;
  int32_t diff;

  /* clear interrupts */
  REG(GPT_2_BASE + GPTIMER_ICR) = 0xffff;

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value) == RADIO_RESULT_OK) {
     rssi = value;
  } else {
     rssi = ~0;
  }

  if(scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL) {
    buf[pos++] = rssi;

    if(pos == RSSI_CHANNEL_COUNT + 5) {
      /* Only switch buffer if previous buffer is empty */
      if(ready_buf == NULL) {
        ready_buf = buf;
        buf = buf == buf_0 ? buf_1  : buf_0;
      }
      pos = 5;
      /* Store time in buffer */
      buf[3] = (timer2a &  0xff00) >> 8;
      buf[4] = (timer2a & 0xff);
    }
  } else {
    /* How much operations can we have with 10 KHz */
    diff = rssi - (average / COUNT);
    average = average + diff;
    variance = variance - (variance / COUNT) + diff * diff;

    /* Experimental code for measuring send-ability on different RSSI */
    if(rssi < -106) {
      /* ok - best case - we do not need to update anything... */
    } else {
      int i;
      int rssi_level = INIT_RSSI;
      for(i = 0; i < BUCKET_COUNT; i++) {
        if(rssi > rssi_level) {
          /* update the expected number of packets that could be sent here -
             assume packets of 4 ms => /10 => 1ms / 40 => 4ms long packets. */
          packet_count[i] += (timer2a - last_sample_time[i]) / 40;
          /* update last send for the lowest level */
          last_sample_time[i] = timer2a;
        }
        /* go lower in limit */
        rssi_level -= BUCKET_WIDTH;
      }
    }
  }
  /* just to know if this works... - sample counter...*/
  timer2a++;
}

static void
init_timer(void)
{
  int i;
  /*
   * Remove the clock gate to enable GPT1 and then initialise it
   */
  REG(SYS_CTRL_RCGCGPT) |= SYS_CTRL_RCGCGPT_GPT2;
  REG(GPT_2_BASE + GPTIMER_CTL) = 0; /* off */
  REG(GPT_2_BASE + GPTIMER_CFG) = 0x04; /* 16 bit */
  REG(GPT_2_BASE + GPTIMER_TAMR) = GPTIMER_TAMR_TAMR_PERIODIC | GPTIMER_TAMR_TAMIE;
  REG(GPT_2_BASE + GPTIMER_TAPR) = 1; /* divisor with 2 */
  REG(GPT_2_BASE + GPTIMER_TAILR) = 800; /* 100us at 32MHz (10 KHz) ??? */
  REG(GPT_2_BASE + GPTIMER_IMR) = GPTIMER_IMR_TATOIM;
  REG(GPT_2_BASE + GPTIMER_CTL) = GPTIMER_CTL_TAEN;

  /* Reset RSSI buffer for new start */
  pos = 5;
  memset(buf_0, 0, sizeof(buf_0));
  memset(buf_1, 0, sizeof(buf_1));
  ready_buf = NULL;

  for(i = 0; i < BUCKET_COUNT; i++) {
    packet_count[i] = 0;
    last_sample_time[i] = 0;
  }
  timer2a = 0;

  is_timer_running = 1;

  /* set up interrupt */
  NVIC_EnableIRQ(GPT2A_IRQn);
}

static void
deinit_timer(void)
{
  /* turn off interrupt */
  NVIC_DisableIRQ(GPT2A_IRQn);
  REG(GPT_2_BASE + GPTIMER_CTL) = 0;

  is_timer_running = 0;
}

/* static uint32_t last_single_scan = 0; */

static void
scan_single_channel(void)
{
  if(scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL) {
    if(ready_buf != NULL) {
      ready_buf[0] = '!';
      ready_buf[1] = 'r';
      ready_buf[2] = 'C';
      cmd_send(ready_buf, 3 + 2 + RSSI_CHANNEL_COUNT);
      ready_buf = NULL;
    }
  } else if(scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL_AGG) {
    if((timer2a & 0xff) == 0xff) {
      int i;
      printf("RSSI Avg: %d, Variance:%d Pkts: %d", (int) (average / COUNT),
             (int) (variance / COUNT), (int) (timer2a / 40));

      for(i = 0; i < BUCKET_COUNT; i++) {
        printf(" %d", packet_count[i]);
      }
      printf("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(scan_process, ev, data)
{
  static struct radio_scan scan_info;
  static struct etimer scan_timer;
  static struct etimer scan_delay_timer;
  static uint8_t active_channel;

  PROCESS_BEGIN();

  while(is_scanning) {
    etimer_set(&scan_timer, DELAY_BETWEEN_SCANS);
    PROCESS_WAIT_UNTIL(etimer_expired(&scan_timer));

    if(scan_mode == RADIO_SCAN_MODE_NORMAL) {
      int8_t rssi;
      radio_value_t value;

      /* Normal scan of selected channels */
      for(active_channel = 11; active_channel <= 26; active_channel++) {
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, active_channel);

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

        add_energy_scan(active_channel,
                        scan_info.rssi_sum,
                        scan_info.rssi_max,
                        scan_info.rssi_min,
                        scan_info.count);
        /*
        buf[0] = '!';
        buf[1] = 'e';
        buf[2] = active_channel;
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
    } else if(scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL ||
              scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL_AGG) {
      /* last_single_scan = 0; */
      init_timer();
      while(is_scanning && (scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL ||
                            scan_mode == RADIO_SCAN_MODE_RSSI_CHANNEL_AGG)) {
        scan_single_channel();
        PROCESS_PAUSE();
      }
      deinit_timer();
      /* etimer_set(&scan_delay_timer, 1); */
      /* PROCESS_WAIT_UNTIL(etimer_expired(&scan_delay_timer)); */
    } else {
      printf("radio scan: unhandled scan mode: %d\n", scan_mode);
      is_scanning = 0;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
