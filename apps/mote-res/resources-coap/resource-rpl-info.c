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
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/link-stats.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static void res_rpl_info_get_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset);
static void res_rpl_parent_get_handler(void *request, void *response,
  uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_rpl_rank_get_handler(void *request, void *response,
  uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_rpl_link_metric_get_handler(void *request, void *response,
  uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/*---------------------------------------------------------------------------*/
RESOURCE(resource_rpl_info,
  "title=\"RPL Information\";obs",
  res_rpl_info_get_handler,
  NULL,
  NULL,
  NULL);
/*---------------------------------------------------------------------------*/
RESOURCE(resource_rpl_parent,
  "title=\"RPL Parent\";obs",
  res_rpl_parent_get_handler,
  NULL,
  NULL,
  NULL);
/*---------------------------------------------------------------------------*/
RESOURCE(resource_rpl_rank,
  "title=\"RPL Rank\";obs",
  res_rpl_rank_get_handler,
  NULL,
  NULL,
  NULL);
/*---------------------------------------------------------------------------*/
RESOURCE(resource_rpl_link_metric,
  "title=\"RPL Link Metric\";obs",
  res_rpl_link_metric_get_handler,
  NULL,
  NULL,
  NULL);
/*---------------------------------------------------------------------------*/
static void res_rpl_parent_get_handler(void *request, void *response,
  uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  uint16_t length;
  rpl_instance_t *instance;
  char message[REST_MAX_CHUNK_SIZE];

  memset(message, 0, REST_MAX_CHUNK_SIZE);
  instance = rpl_get_instance(RPL_DEFAULT_INSTANCE);
  /* Print the parent */
  if(instance != NULL &&
    instance->current_dag != NULL &&
    instance->current_dag->preferred_parent != NULL) {
    sprintf(&message[0], "RPL Parent: ");
    sprint_addr6(&message[0],
      rpl_get_parent_ipaddr(instance->current_dag->preferred_parent));
  } else {
    sprintf(&message[0], "No parent yet\n");
  }
  length = strlen(&message[0]);
  memcpy(buffer, message, length);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *)&length, 1);
  REST.set_response_payload(response, buffer, length);
}
/*---------------------------------------------------------------------------*/
static void res_rpl_rank_get_handler(void *request, void *response,
  uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  uint16_t length;
  rpl_instance_t *instance;
  char message[REST_MAX_CHUNK_SIZE];

  memset(message, 0, REST_MAX_CHUNK_SIZE);
  instance = rpl_get_instance(RPL_DEFAULT_INSTANCE);
  /* Print the RPL rank */
  if(instance != NULL && instance->current_dag != NULL) {
    snprintf(message, sizeof(message) - 1, "RPL Rank: %u.%02u",
             instance->current_dag->rank / 128,
             (int)((100L * instance->current_dag->rank) / 128) % 100);
  } else {
    snprintf(message, sizeof(message) - 1, "No rank yet");
  }
  length = strlen(&message[0]);
  memcpy(buffer, message, length);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *)&length, 1);
  REST.set_response_payload(response, buffer, length);
}
/*---------------------------------------------------------------------------*/
static void res_rpl_link_metric_get_handler(void *request, void *response,
  uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  rpl_instance_t *instance;
  uint16_t length;
  uint16_t link_metric;
  char message[REST_MAX_CHUNK_SIZE];

  memset(message, 0, REST_MAX_CHUNK_SIZE);
  instance = rpl_get_instance(RPL_DEFAULT_INSTANCE);
  /* Print the link metric to parent */
  if(instance != NULL &&
     instance->current_dag != NULL &&
     instance->current_dag->preferred_parent != NULL) {
    link_metric = rpl_get_parent_link_metric(instance->current_dag->preferred_parent);
    snprintf(message, sizeof(message) - 1, "Link Metric: %u.%02u\n",
             link_metric / LINK_STATS_ETX_DIVISOR,
             (int)((link_metric * 100L) / LINK_STATS_ETX_DIVISOR) % 100);
  } else {
    snprintf(message, sizeof(message) - 1, "No link metric yet\n");
  }
  length = strlen(message);
  memcpy(buffer, message, length);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *)&length, 1);
  REST.set_response_payload(response, buffer, length);
}
/*---------------------------------------------------------------------------*/
static void
res_rpl_info_get_handler(void *request, void *response, uint8_t *buffer,
  uint16_t preferred_size, int32_t *offset)
{
  /*Return RPL information in JSON format */
  uint16_t length = 0;
  uint16_t link_metric;
  rpl_instance_t *instance;
  char message[REST_MAX_CHUNK_SIZE];
  memset(message, 0, sizeof(message));

  instance = rpl_get_instance(RPL_DEFAULT_INSTANCE);

  /* Write all RPL info in JSON format */
  snprintf(message, sizeof(message) - 1, "{\"parent\":\"");
  length = strlen(message);

  if(instance != NULL &&
    instance->current_dag != NULL &&
    instance->current_dag->preferred_parent != NULL) {
    sprint_addr6(&message[length],
                 rpl_get_parent_ipaddr(instance->current_dag->preferred_parent));
    length = strlen(message);
    snprintf(&message[length], sizeof(message) - length - 1, "\"");
  } else {
    snprintf(&message[length], sizeof(message) - length - 1, "None\"");
  }
  length = strlen(message);

  snprintf(&message[length], sizeof(message) - length - 1, " ,\"rank\":\"");
  length = strlen(message);

  if(instance != NULL && instance->current_dag != NULL) {
    snprintf(&message[length], sizeof(message) - length - 1, "%u.%02u\"",
             (instance->current_dag->rank / 128),
             (int)((100L * instance->current_dag->rank) / 128) % 100);
  } else {
    snprintf(&message[length], sizeof(message) - length - 1, "inf\"");
  }
  length = strlen(message);

  snprintf(&message[length], sizeof(message) - length - 1, " ,\"link-metric\":\"");
  length = strlen(message);
  if(instance != NULL &&
     instance->current_dag != NULL &&
     instance->current_dag->preferred_parent != NULL) {
    link_metric = rpl_get_parent_link_metric(instance->current_dag->preferred_parent);
    snprintf(&message[length], sizeof(message) - length - 1, "%u.%02u\"",
             link_metric / LINK_STATS_ETX_DIVISOR,
             (int)((link_metric * 100L) / LINK_STATS_ETX_DIVISOR) % 100);
  } else {
    snprintf(&message[length], sizeof(message) - length - 1, "inf\"");
  }
  length = strlen(message);
  snprintf(&message[length], sizeof(message) - length - 1, "}");
  length = strlen(message);
  memcpy(buffer, message, length);

  REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
  REST.set_header_etag(response, (uint8_t *)&length, 1);
  REST.set_response_payload(response, buffer, length);
}
/*---------------------------------------------------------------------------*/
