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
#include <stdio.h>
#include <stdlib.h>
#include "resource-print-engine.h"
#include "rpl.h"
#include "rpl-private.h"
/*---------------------------------------------------------------------------*/
uint16_t
print_rpl_parent(char *msg, int max_chars)
{
  rpl_instance_t *instance;
  instance = rpl_get_instance(RPL_DEFAULT_INSTANCE);
  if(instance != NULL &&
    instance->current_dag != NULL &&
    instance->current_dag->preferred_parent != NULL) {
    sprint_addr6(msg,
      rpl_get_parent_ipaddr(instance->current_dag->preferred_parent));
  } else {
    snprintf(msg, max_chars, "Null");
  }
  return strlen(msg);
}
/*---------------------------------------------------------------------------*/
uint16_t
print_rpl_rank(char *msg, int max_chars)
{
  rpl_instance_t *instance;
  instance = rpl_get_instance(RPL_DEFAULT_INSTANCE);
  if(instance != NULL && instance->current_dag != NULL) {
    snprintf(msg, max_chars, "%u.%02u", 
      instance->current_dag->rank / RPL_DAG_MC_ETX_DIVISOR,
      (100 * instance->current_dag->rank % RPL_DAG_MC_ETX_DIVISOR) /
      RPL_DAG_MC_ETX_DIVISOR);
  } else {
    snprintf(msg, max_chars, "inf");
  }
  return strlen(msg);
}
/*---------------------------------------------------------------------------*/
uint16_t
print_rpl_link_metric(char *msg, int max_chars)
{
  rpl_instance_t *instance;
  uip_ds6_nbr_t *nbr;
  instance = rpl_get_instance(RPL_DEFAULT_INSTANCE);
  if(instance != NULL &&
    instance->current_dag != NULL &&
    instance->current_dag->preferred_parent != NULL &&
    (nbr = rpl_get_nbr(instance->current_dag->preferred_parent)) != NULL) {
    snprintf(msg, max_chars, "%u.%02u\n",
      nbr->link_metric / RPL_DAG_MC_ETX_DIVISOR,
      (100 * (nbr->link_metric % RPL_DAG_MC_ETX_DIVISOR)) /
      RPL_DAG_MC_ETX_DIVISOR);
  } else {
    snprintf(msg, max_chars, "inf");
  }
  return strlen(msg);
}
/*---------------------------------------------------------------------------*/
