/*
 * Copyright (c) 2015, Yanzi Networks AB.
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
 *
 */

#include "contiki-net.h"
#include "rest-engine.h"
#include "er-coap.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dev/leds.h"

#ifdef RESOURCE_LED0_CONF_LED
#define RESOURCE_LED0 RESOURCE_LED0_CONF_LED
#endif

#ifdef RESOURCE_LED1_CONF_LED
#define RESOURCE_LED1 RESOURCE_LED1_CONF_LED
#endif

#ifdef RESOURCE_LED2_CONF_LED
#define RESOURCE_LED2 RESOURCE_LED2_CONF_LED
#endif

static uint8_t resource_led_mask[] = {
#ifdef RESOURCE_LED0
  RESOURCE_LED0,
#endif
#ifdef RESOURCE_LED1
  RESOURCE_LED1,
#endif
#ifdef RESOURCE_LED2
  RESOURCE_LED2,
#endif
};

static uint32_t resource_led_max = (sizeof(resource_led_mask) / sizeof(uint8_t));

#define GET_LED_RES_INDEX(req, uripath)                                            \
  do {                                                                             \
    if(resource_led_max == 0)                                                      \
      return;                                                                      \
    else if(coap_get_header_uri_path(req, (const char **)&uripath)) {              \
      const char *p;                                                               \
      if((p = strstr(uri_path, "led/"))) {                                         \
        led_index = *(p + 4) - '0';                                                \
        if(led_index > resource_led_max)                                           \
          return;                                                                  \
      }                                                                            \
      else if((p = strstr(uri_path, "led")))                                       \
        led_index = 0;                                                             \
      else                                                                         \
        return;                                                                    \
    }                                                                              \
    else                                                                           \
      return;                                                                      \
  } while(0)

static void res_get_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset);
static void res_post_handler(void *request, void *response,
  uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_put_handler(void *request, void *response,
  uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/*---------------------------------------------------------------------------*/
/*
 * Declare the LED resources for the iot-u10.
 */
#ifdef RESOURCE_LED0
RESOURCE(resource_led,
  "title=\"LED(default): led=toggle ?len=0..\";rt=\"Text\"",
  res_get_handler,
  res_post_handler,
  res_put_handler,
  NULL);

RESOURCE(resource_led0,
  "title=\"LED0: led=toggle ?len=0..\";rt=\"Text\"",
  res_get_handler,
  res_post_handler,
  res_put_handler,
  NULL);
#endif
/*---------------------------------------------------------------------------*/
#ifdef RESOURCE_LED1
RESOURCE(resource_led1,
  "title=\"LED1: led=toggle ?len=0..\";rt=\"Text\"",
  res_get_handler,
  res_post_handler,
  res_put_handler,
  NULL);
#endif
/*---------------------------------------------------------------------------*/
#ifdef RESOURCE_LED2
RESOURCE(resource_led2,
  "title=\"LED2: led=toggle ?len=0..\";rt=\"Text\"",
  res_get_handler,
  res_post_handler,
  res_put_handler,
  NULL);
#endif
/*---------------------------------------------------------------------------*/
static void
res_get_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset)
{
  int led_index;
  const char *uri_path = NULL;
  const char *len = NULL;
  char message[REST_MAX_CHUNK_SIZE];

  GET_LED_RES_INDEX(request, uri_path);

  memset(message, 0, REST_MAX_CHUNK_SIZE);
  if((leds_get() & resource_led_mask[led_index]) != 0) {
    sprintf(&message[0], "LED%d ON", led_index);
  } else {
    sprintf(&message[0], "LED%d OFF", led_index);
  }
  int length = strlen(&message[0]);

  /* The query string can be retrieved by rest_get_query(),
   * or parsed for its key-value pairs.
   */
  if(REST.get_query_variable(request, "len", &len)) {
    length = atoi(len);
    if(length < 0) {
      length = 0;
    }
    if(length > REST_MAX_CHUNK_SIZE) {
      length = REST_MAX_CHUNK_SIZE;
    }
    memcpy(buffer, message, length);
  } else {
    memcpy(buffer, message, length);
  }
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *)&length, 1);
  REST.set_response_payload(response, buffer, length);
}
/*---------------------------------------------------------------------------*/
static void
res_post_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset)
{
  int led_index;
  const char *uri_path = NULL;
  int err = 0;
  char response_message[REST_MAX_CHUNK_SIZE];

  GET_LED_RES_INDEX(request, uri_path);

  memset(&response_message[0], 0, REST_MAX_CHUNK_SIZE);
  uint8_t length = 0;

  leds_toggle(resource_led_mask[led_index]);
  if((leds_get() & resource_led_mask[led_index]) != 0) {
    sprintf(&response_message[0], "LED%d Toggle: ON", led_index);
  } else {
    sprintf(&response_message[0], "LED%d Toggle: OFF", led_index);
  }
  length = strlen(&response_message[0]);

  if(err) {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  } else {
    memcpy(buffer, &response_message[0], length);
  }
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *)&length, 1);
  REST.set_response_payload(response, buffer, length);
}
/*---------------------------------------------------------------------------*/
static void
res_put_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset)
{
  int led_index;
  const char *uri_path = NULL;
  size_t len = 0;
  int err = 0;
  const char *led = NULL;
  char response_message[REST_MAX_CHUNK_SIZE];

  GET_LED_RES_INDEX(request, uri_path);

  memset(&response_message[0], 0, REST_MAX_CHUNK_SIZE);

  uint8_t length = 0;
  if((len = coap_get_payload(request, (const uint8_t **)&led))) {
    if(strncmp(led, "0", len) == 0) {
      /* Set LED to OFF */
      leds_off(resource_led_mask[led_index]);
      sprintf(&response_message[0], "LED%d OFF", led_index);
      length = strlen(&response_message[0]);
    } else if(strncmp(led, "1", len) == 0) {
      /* Set LED to ON */
      leds_on(resource_led_mask[led_index]);
      sprintf(&response_message[0], "LED%d ON", led_index);
      length = strlen(&response_message[0]);
    } else {
      /* Unrecognized put request */
      err++;
    }
  }
  if(err) {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  } else {
    memcpy(buffer, &response_message[0], length);
  }
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *)&length, 1);
  REST.set_response_payload(response, buffer, length);
}
/*---------------------------------------------------------------------------*/
