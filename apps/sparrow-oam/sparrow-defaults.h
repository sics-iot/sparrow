/*
 * Copyright (c) 2015-2016, Yanzi Networks AB.
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 *         Default network configuration for SPARROW devices
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef SPARROW_DEFAULTS_H_
#define SPARROW_DEFAULTS_H_

#if NETSTACK_CONF_WITH_IPV6 == 0
#error IPv6 is not enabled
#endif /* NETSTACK_CONF_WITH_IPV6 == 0 */

#if UIP_CONF_BUFFER_SIZE < 1280
#error UIP_BUFFER_SIZE is less than 1280
#endif /* UIP_CONF_BUFFER_SIZE */

#define LINKADDR_CONF_SIZE                           8
#define UIP_CONF_LL_802154                           1
#define UIP_CONF_LLH_LEN                             0

#define UIP_DS6_CONF_PERIOD                          CLOCK_SECOND

/* Should this be enabled for the nodes? */
/* #define UIP_CONF_DS6_LL_NUD                          1 */

#define UIP_CONF_ICMP6                               1
#define UIP_CONF_UDP                                 1

/* Configure DAO routes to have a lifetime of 30 x 60 seconds */
#define RPL_CONF_DEFAULT_LIFETIME_UNIT              60
#define RPL_CONF_DEFAULT_LIFETIME                   30

#define RPL_CONF_PARENT_SWITCH_THRESHOLD rpl_nbr_policy_parent_switch_treshold

#define RPL_CONF_INSERT_HBH_OPTION                   1

#define RPL_CONF_DAO_ACK                             1

/* Need to enable NA */
#define UIP_CONF_ND6_SEND_NA                         1
#define UIP_CONF_ND6_REACHABLE_TIME             3600000L

/*
 * The ETX in the metric container is expressed as a fixed-point value whose
 * integer part can be obtained by dividing the value by RPL_DAG_MC_ETX_DIVISOR
 * and the RPL ETX divisor should be 128 according to RFC.
 */
/* #define RPL_DAG_MC_ETX_DIVISOR                     128 */
/* #define RPL_CONF_MIN_HOPRANKINC                    128 */

#define RPL_CONF_MAX_DAG_PER_INSTANCE                1

/*
 * Contiki RPL has a builtin probing mechanism but sparrow devices use a
 * different mechanism.
 */
#define RPL_CONF_WITH_PROBING                        0

/* Default route should have infinite lifetime */
#define RPL_CONF_DEFAULT_ROUTE_INFINITE_LIFETIME     1

#define RPL_CONF_NOPATH_REMOVAL_DELAY               60

#define SICSLOWPAN_CONF_MAC_MAX_PAYLOAD      (127 - 2)
#define SICSLOWPAN_CONF_COMPRESSION_THRESHOLD        0
#define SICSLOWPAN_CONF_FRAG                         1
/* Timeout for packet reassembly at the 6LoWPAN layer (1/16 seconds) */
#define SICSLOWPAN_CONF_MAXAGE                      16
#define SICSLOWPAN_CONF_COMPRESSION                  SICSLOWPAN_COMPRESSION_HC06

#if SICSLOWPAN_CONF_FRAGMENT_BUFFERS < 14
#error SICSLOWPAN should have at least 14 fragmentation buffers
#endif

#define NETSTACK_CONF_USING_QUEUEBUF                 0

#endif /* SPARROW_DEFAULTS_H_ */
