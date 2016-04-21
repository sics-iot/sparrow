/*
 * Copyright (c) 2016, Yanzi Networks AB.
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
 */

#include "contiki.h"
#include "dev/serial-line.h"
#include "dev/leds-ext.h"
#include "dev/sparrow-device.h"
#include "dev/button-sensor.h"
#include <stdlib.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

PROCESS(sparrow_main, "sparrow main");
AUTOSTART_PROCESSES(&sparrow_main);

PROCESS_THREAD(sparrow_main, ev, data)
{
  static struct etimer timer;
  static unsigned count;

  PROCESS_BEGIN();

  SENSORS_ACTIVATE(button_sensor);

  etimer_set(&timer, CLOCK_SECOND * 10);
  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_TIMER) {
      etimer_restart(&timer);
      count++;
      PRINTF("alive %lu sec\n", clock_seconds());
    } else if(ev == sensors_event) {
      if(data == &button_sensor) {
        PRINTF("user button pressed: %d\n", button_sensor.value(0));
      }
    } else if(ev == serial_line_event_message) {
      const char *line;
      line = data;

      if(*line == '\0') {
        /* Ignore empty lines */
      } else if(*line == 'r') {
        line++;
        if(*line == '1' || *line == '2') {
          PRINTF("Rebooting to image %d\n", *line - '0');
          clock_delay_usec(500);
          SPARROW_DEVICE.reboot_to_image(*line - '0');
        } else if(*line == '0') {
          PRINTF("Rebooting\n");
          clock_delay_usec(500);
          SPARROW_DEVICE.reboot();
        } else {
          PRINTF("Please specify 0, 1, or 2\n");
        }
#if PLATFORM_HAS_LEDS_EXT
      } else if(*line == 'l') {
        int i;

        if(*(line + 1) != '\0' && leds_count > 0) {
          i = atoi(line + 1);
          leds_set_state(leds[0], i);
        }

        PRINTF("LEDS:\n");
        for(i = 0; i < leds_count; i++) {
          PRINTF("  led %d is in state %u / %d\n", (i + 1),
                 leds_get_state(leds[i]), leds_get_state_count(leds[i]));
        }
#endif /* PLATFORM_HAS_LEDS_EXT */
      } else {
        PRINTF("Unknown command\n");
      }
    }
  }

  PROCESS_END();
}
