/*
 * Copyright (c) 2010-2016, Swedish Institute of Computer Science.
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

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/*
 * Custom configuration for platform Zoul
 */
#ifdef CONTIKI_TARGET_ZOUL_SPARROW

/* Use UART by default on Zoul platforms unless USB_DEBUG_PORT is set */
#ifndef WITH_UART
#if defined(ZOUL_SPARROW_USB_DEBUG_PORT) || defined(SLIP_ARCH_CONF_USB)
#define WITH_UART 0
#else
#define WITH_UART 1
#endif /* ZOUL_SPARROW_USB_DEBUG_PORT */
#endif /* WITH_UART */

#endif /* CONTIKI_TARGET_ZOUL_SPARROW */


/*
 * Custom configuration for CC2538 based platforms
 */
#ifdef PLATFORM_HAS_CC2538

/* LPM needs to be enabled for select the 32MHz clock */
#define LPM_CONF_ENABLE          1
#define SERIAL_BAUDRATE     460800

#ifndef WITH_UART
#define WITH_UART 0
#endif

#if WITH_UART

#define DBG_CONF_USB             0 /* command output uses debug output */
#define SLIP_ARCH_CONF_USB       0

#else /* WITH_UART */

#define DBG_CONF_USB             1 /* command output uses debug output */
#define SLIP_ARCH_CONF_USB       1

#define USB_CONF_WRITE_TIMEOUT   0 /* no USB write timeout in serial radio */

/* Use different product description for USB */
#define USB_SERIAL_GET_PRODUCT_DESCRIPTION serial_radio_get_product_description

#endif /* WITH_UART */

#endif /* PLATFORM_HAS_CC2538 */


/* Get rid of address filter in NULLRDC - assume radio filter */
#define NULLRDC_CONF_ADDRESS_FILTER      0

#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES              1

#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS     1

#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM                4

#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE          1280

#undef UIP_CONF_ROUTER
#define UIP_CONF_ROUTER                  0

#define CMD_CONF_OUTPUT serial_radio_cmd_output
#define CMD_CONF_ERROR  serial_radio_cmd_error

/* configuration for the serial radio/network driver */
#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC     nullmac_driver

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     sniffer_rdc_driver

#undef NETSTACK_CONF_NETWORK
#define NETSTACK_CONF_NETWORK encnet_driver

#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER  no_framer

/*
 * OAM definitions
 */
#define PRODUCT_TYPE_INT64 0x0090DA0301010482ULL
#define PRODUCT_LABEL "Serial Radio"

#define SERIAL_RADIO_CONTROL_API_VERSION 3L

#endif /* PROJECT_CONF_H_ */
