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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "rest-engine.h"
#include "er-coap.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define SPRINT6ADDR(addr) sprint_addr6(buf, addr)
#else
#define PRINTF(...)
#define SPRINT6ADDR(addr)
#endif

static uint32_t event_counter = 0;
/*---------------------------------------------------------------------------*/
static void res_get_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);
/*---------------------------------------------------------------------------*/
/*
 * Event CoAP resource definition.
 */
EVENT_RESOURCE(resource_push_button_event,
  "title=\"Push event\";obs",
  res_get_handler,
  NULL,
  NULL,
  NULL,
  res_event_handler);
/*---------------------------------------------------------------------------*/
static void
res_get_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset)
{
  char message[REST_MAX_CHUNK_SIZE];
  memset(message, 0, REST_MAX_CHUNK_SIZE);
  snprintf(message, REST_MAX_CHUNK_SIZE - 1, "Push event no: %lu\n",
           (unsigned long)event_counter);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  memcpy(buffer, &message[0], strlen(message));
  REST.set_response_payload(response, buffer, strlen(message));
}
/*---------------------------------------------------------------------------*/
static void
res_event_handler(void)
{
  /* Increment event counter */
  ++event_counter;

  /* Notify the registered observers which will trigger
   * the res_get_handler to create the response.
   */
  REST.notify_subscribers(&resource_push_button_event);
}
/*---------------------------------------------------------------------------*/
