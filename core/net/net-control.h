/*
 * Copyright (c) 2016, SICS Swedish ICT AB
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *         Network control API
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef NET_CONTROL_H_
#define NET_CONTROL_H_

#include "contiki-conf.h"

typedef uint8_t net_control_key_t;

#define NET_CONTROL_KEY_NONE 0

typedef enum {
  NET_CONTROL_RPL_SUPPRESS_DIO,
  NET_CONTROL_SUPPRESS_BEACON,
  NET_CONTROL_MAX
} net_control_flag_t;


/**
 * Allocate a new Network control key.
 *
 * \retval A Net control key or NET_CONTROL_KEY_NONE if no more keys
 * can be allocated.
 */
net_control_key_t net_control_alloc_key(void);

/**
 * Get the stat of a flag
 *
 * \retval The DIO suppression state
 */
int net_control_get(net_control_flag_t flag);

/**
 * Set the DIO suppression state for the specified RPL control
 * key. All DIOs are suppressed as long as suppress state for at least
 * one RPL control key has been set.
 *
 * \param flag The Net control flag to use to set the flag
 * \param key The Net control key to use to set the flag
 * \param state Non-zero to set the flag
 */
void net_control_set(net_control_flag_t flag, net_control_key_t key, int state);

#endif /* RPL_CONTROL_H_ */
