/*
 * Copyright (c) 2015-2016, Yanzi Networks AB.
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
#if WITH_WEBSERVER
#include "node-webserver-simple.h"
#endif
#include "resources-coap.h"
#include "dev/button-sensor.h"
#include "dev/slide-switch.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

PROCESS(felicia_main, "felicia main");
AUTOSTART_PROCESSES(&felicia_main);

PROCESS_THREAD(felicia_main, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  /* Initialize the CoAP server and activate resources */
  rest_init_engine();
#if PLATFORM_HAS_LEDS
  rest_activate_resource(&resource_led, (char *)"led");
  rest_activate_resource(&resource_led0, (char *)"led/0");
  rest_activate_resource(&resource_led1, (char *)"led/1");
#endif
  rest_activate_resource(&resource_ipv6_neighbors, (char *)"ipv6/neighbors");
  rest_activate_resource(&resource_rpl_info, (char *)"rpl-info");
  rest_activate_resource(&resource_rpl_parent, (char *)"rpl-info/parent");
  rest_activate_resource(&resource_rpl_rank, (char *)"rpl-info/rank");
  rest_activate_resource(&resource_rpl_link_metric, (char *)"rpl-info/link-metric");

#if PLATFORM_HAS_SLIDE_SWITCH
    SENSORS_ACTIVATE(slide_switch_sensor);
#endif

#if PLATFORM_HAS_BUTTON
    SENSORS_ACTIVATE(button_sensor);
    rest_activate_resource(&resource_push_button_event, (char *)"push-button");
#endif

#if PLATFORM_HAS_SENSORS
    rest_activate_resource(&resource_temperature, (char *)"temperature");
#endif

#if WITH_WEBSERVER
  PROCESS_PAUSE();
  process_start(&node_webserver_simple_process, NULL);
#endif

  while(1) {
    etimer_set(&timer, CLOCK_SECOND * 5);
    PROCESS_WAIT_EVENT();

#if PLATFORM_HAS_BUTTON
      if(ev == sensors_event && data == &button_sensor) {
        resource_push_button_event.trigger();
        PRINTF("Button pressed!\n");
      }
#endif

#if PLATFORM_HAS_SLIDE_SWITCH
      if(ev == sensors_event && data == &slide_switch_sensor) {
        PRINTF("Sliding switch is %s\n",
               slide_switch_sensor.value(0) ? "on" : "off");
      }
#endif
  }

  PROCESS_END();
}
