/* Copyright (c) 2015, Yanzi Networks AB.
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

#include "resources-coap.h"
#include "contiki-net.h"
#include "rest-engine.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if PLATFORM_HAS_SENSORS
#include "stts751.h"
#endif

static void res_get_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset);

/*---------------------------------------------------------------------------*/
static uint16_t
print_temperature(char *msg)
{
#if PLATFORM_HAS_SENSORS
  uint32_t millikelvin = stts751_millikelvin();
  int32_t celcius_int_part = millikelvin/1000-273;
  uint8_t celcius_dec_part = (millikelvin/100) - (millikelvin/1000)*10;
  sprintf(msg+strlen(msg),"Temperature (C): %2ld.%u",
    celcius_int_part, celcius_dec_part);
#else
  sprintf(msg+strlen(msg),"Temperature (C): %2ld.%u",
    0, 0);
#endif

  return strlen(msg);
}
/*---------------------------------------------------------------------------*/
/*
 * Declare the IPv6 neighbors resource 
 */
RESOURCE(resource_temperature,
  "title=\"Temperature: ?len=0..\";rt=\"Text\"",
  res_get_handler,
  NULL,
  NULL,
  NULL);
/*---------------------------------------------------------------------------*/
static void
res_get_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset)
{
  const char *len = NULL;
  char message[40];
  memset(message, 0, 40);
  print_temperature(&message[0]);
  
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

