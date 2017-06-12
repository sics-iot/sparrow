/*
 * Copyright (c) 2011-2016, Swedish Institute of Computer Science.
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
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef PROJECT_ROUTER_CONF_H_
#define PROJECT_ROUTER_CONF_H_

#ifndef WITH_CLOCK_GETTIME
#ifdef linux
#define WITH_CLOCK_GETTIME 1
#else /* linux */
#define WITH_CLOCK_GETTIME 0
#endif /* linux */
#endif /* WITH_CLOCK_GETTIME */

#if WITH_CLOCK_GETTIME
/* Use clock_gettime() under Linux for monotonic time */
#define CLOCK_USE_CLOCK_GETTIME 1
#define CLOCK_USE_RELATIVE_TIME 0
#else
#define CLOCK_USE_CLOCK_GETTIME 0
#define CLOCK_USE_RELATIVE_TIME 1
#endif

#define SELECT_CONF_MAX 16

#ifndef RF_CHANNEL
#define RF_CHANNEL 26
#endif

#define UIP_DS6_ROUTE_FIND_REMOVABLE rpl_route_find_removable_highage

#undef UIP_FALLBACK_INTERFACE
#define UIP_FALLBACK_INTERFACE rpl_interface

#define UIP_CONF_DS6_LL_NUD 1

#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM         32


#ifdef WITH_SUBGHZ
/* If using 802.15.4g - we need to set later packet sizes */
#define PACKETBUF_CONF_SIZE             255
#define SICSLOWPAN_CONF_MAC_MAX_PAYLOAD 253
#endif

#undef SICSLOWPAN_CONF_FRAGMENT_BUFFERS
#define SICSLOWPAN_CONF_FRAGMENT_BUFFERS 64

#undef SICSLOWPAN_CONF_REASS_CONTEXTS
#define SICSLOWPAN_CONF_REASS_CONTEXTS   16

#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    1280

#undef UIP_CONF_RECEIVE_WINDOW
#define UIP_CONF_RECEIVE_WINDOW  60

#define ENC_DEV_CONF_SEND_DELAY (CLOCK_SECOND / 64)
/* #define ENC_DEV_CONF_SEND_DELAY (CLOCK_SECOND / 128) */

#undef WEBSERVER_CONF_CFS_CONNS
#define WEBSERVER_CONF_CFS_CONNS 2

#define CMD_CONF_OUTPUT border_router_cmd_output
#define CMD_CONF_ERROR  border_router_cmd_error

#undef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO border_router_radio_driver

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC border_router_rdc_driver

/* Configure DAO routes to have a lifetime of 30 x 60 seconds */
#define RPL_CONF_DEFAULT_LIFETIME_UNIT 60
#define RPL_CONF_DEFAULT_LIFETIME 30

#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 1000

#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 2500

#undef UIP_CONF_DS6_ROUTE_NBU
#define UIP_CONF_DS6_ROUTE_NBU 2500

/* used by wpcap (see /cpu/native/net/wpcap-drv.c) */
#define SELECT_CALLBACK 1

/* Default network configuration for Sparrow devices */
#include "sparrow-defaults.h"

/* OAM definitions */
#define PRODUCT_TYPE_INT64 0x0090DA0302010014ULL

#define SPARROW_OAM_PORT 49111
#define PRODUCT_LABEL "border router"
#define SUPPORTED_RADIO_TYPE 0x0090DA0301010482ULL

#define RADIO_INSTANCES 3
#define SPARROW_OAM_INSTANCE0_NAME instance_br

#define BORDER_ROUTER_CONTROL_API_VERSION 3

/*
 * Set the radio watchdog in seconds as long as unit controller is
 * running but not peered.
 */
#define BORDER_ROUTER_CONF_SET_RADIO_WATCHDOG (10 * 60)

#define BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_OK      (7)
#define BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING (0)

/* The border router will never push network statistics */
#define INSTANCE_NSTATS_WITH_PUSH 0

#include "instance-brm.h"

#define VARGEN_ADDITIONAL_INSTANCES INSTANCE_BRM_VARIABLES

/* Network statistics */
#define RPL_CONF_STATS 1
#define NETSELECT_CONF_STATS 1
/* #define HANDLER_802154_CONF_STATS 1 */

#define LATENCY_STATISTICS 1

#if LLSEC_CONF_LEVEL
#undef LLSEC802154_CONF_ENABLED
#define LLSEC802154_CONF_ENABLED          1
#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER              noncoresec_framer
#undef NETSTACK_CONF_LLSEC
#define NETSTACK_CONF_LLSEC               noncoresec_driver
#undef NONCORESEC_CONF_SEC_LVL
#define NONCORESEC_CONF_SEC_LVL           LLSEC_CONF_LEVEL

#define NONCORESEC_CONF_KEY { 0x00 , 0x01 , 0x02 , 0x03 , \
                              0x04 , 0x05 , 0x06 , 0x07 , \
                              0x08 , 0x09 , 0x0A , 0x0B , \
                              0x0C , 0x0D , 0x0E , 0x0F }
#endif /* LLSEC_CONF_LEVEL */

#endif /* PROJECT_ROUTER_CONF_H_ */
