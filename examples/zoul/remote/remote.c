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
#if WITH_WEBSERVER
#include "node-webserver-simple.h"
#endif
#if PLATFORM_HAS_SENSORS
#include "dev/zoul-sensors.h"
#endif
#if WITH_IPSO
#include "lwm2m-engine.h"
#include "ipso-objects.h"
#endif
#include "resources-coap.h"
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

#if WITH_IPSO
#ifndef REGISTER_WITH_LWM2M_BOOTSTRAP_SERVER
#define REGISTER_WITH_LWM2M_BOOTSTRAP_SERVER 1
#endif

#ifndef REGISTER_WITH_LWM2M_SERVER
#define REGISTER_WITH_LWM2M_SERVER 1
#endif

#ifndef LWM2M_SERVER_ADDRESS
#define LWM2M_SERVER_ADDRESS "fd00::1"
#endif

/*---------------------------------------------------------------------------*/
static void
setup_lwm2m_servers(void)
{
#ifdef LWM2M_SERVER_ADDRESS
  uip_ipaddr_t addr;
  if(uiplib_ipaddrconv(LWM2M_SERVER_ADDRESS, &addr)) {
    lwm2m_engine_register_with_bootstrap_server(&addr, 0);
    lwm2m_engine_register_with_server(&addr, 0);
  }
#endif /* LWM2M_SERVER_ADDRESS */

  lwm2m_engine_use_bootstrap_server(REGISTER_WITH_LWM2M_BOOTSTRAP_SERVER);
  lwm2m_engine_use_registration_server(REGISTER_WITH_LWM2M_SERVER);
}
/*---------------------------------------------------------------------------*/
#endif
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sparrow_main, ev, data)
{
  static struct etimer timer;
  static unsigned count;

  PROCESS_BEGIN();

 /* Initialize the CoAP server and activate resources */
  rest_init_engine();
#if PLATFORM_HAS_LEDS
#ifdef  RESOURCE_LED0_CONF_LED
  rest_activate_resource(&resource_led, (char *)"led");
  rest_activate_resource(&resource_led0, (char *)"led/0");
#endif
#ifdef  RESOURCE_LED1_CONF_LED
  rest_activate_resource(&resource_led1, (char *)"led/1");
#endif
#ifdef  RESOURCE_LED2_CONF_LED
  rest_activate_resource(&resource_led2, (char *)"led/2");
#endif
#endif
  rest_activate_resource(&resource_ipv6_neighbors, (char *)"ipv6/neighbors");
  rest_activate_resource(&resource_rpl_info, (char *)"rpl-info");
  rest_activate_resource(&resource_rpl_parent, (char *)"rpl-info/parent");
  rest_activate_resource(&resource_rpl_rank, (char *)"rpl-info/rank");
  rest_activate_resource(&resource_rpl_link_metric, (char *)"rpl-info/link-metric");

#if PLATFORM_HAS_BUTTON
  SENSORS_ACTIVATE(button_sensor);
  rest_activate_resource(&resource_push_button_event, (char *)"push-button");
#endif

#if PLATFORM_HAS_SENSORS
  SENSORS_ACTIVATE(cc2538_temp_sensor);
  rest_activate_resource(&resource_temperature, (char *)"temperature");
#endif

  PROCESS_PAUSE();

#if WITH_WEBSERVER
  process_start(&node_webserver_simple_process, NULL);
#endif

#if WITH_IPSO
  /* Initialize the OMA LWM2M engine */
  lwm2m_engine_init();

  /* Register default LWM2M objects */
  lwm2m_engine_register_default_objects();

  /* Register default IPSO objects */
  ipso_objects_init();

  setup_lwm2m_servers();
#endif

  etimer_set(&timer, CLOCK_SECOND * 10);
  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_TIMER) {
      etimer_restart(&timer);
      count++;
      PRINTF("alive %lu sec\n", clock_seconds());
    } else if(ev == sensors_event) {
#if PLATFORM_HAS_BUTTON
      if(data == &button_sensor) {
        resource_push_button_event.trigger();
        PRINTF("user button pressed: %d\n", button_sensor.value(0));
      }
#endif
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
