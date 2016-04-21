/*
 * Copyright (c) 2015-2016, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of the copyright holders nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
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

/**
 * \file
 *         Simple netscan selector that joins any network with the
 *         pre-configured PAN ID.
 *
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "netscan.h"
#include "net/rpl/rpl.h"
#include "net/mac/frame802154.h"
/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
static int
is_done(void)
{
#if UIP_CONF_IPV6_RPL
  return rpl_get_any_dag() != NULL;
#else /* UIP_CONF_IPV6_RPL */
  return 1;
#endif /* UIP_CONF_IPV6_RPL */
}
/*---------------------------------------------------------------------------*/
static netscan_action_t
beacon_received(int channel, uint16_t panid, const linkaddr_t *source,
                const uint8_t *payload, uint8_t payload_length)
{
  if(panid == IEEE802154_PANID) {
    netscan_select_network(channel, panid);
    netscan_set_beacon_payload(payload, payload_length);
    return NETSCAN_STOP;
  }
  return NETSCAN_CONTINUE;
}
/*---------------------------------------------------------------------------*/
static netscan_action_t
channel_done(int channel, int is_last_channel)
{
  return NETSCAN_CONTINUE;
}
/*---------------------------------------------------------------------------*/
const struct netscan_selector netscan_default_selector = {
  init,
  is_done,
  beacon_received,
  channel_done
};
/*---------------------------------------------------------------------------*/
