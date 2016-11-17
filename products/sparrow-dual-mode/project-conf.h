/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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

/* #define USB_SERIAL_CONF_ENABLE   1 */ /* Needed in IoT-U10 remote mode */
#define SLIP_ARCH_CONF_USB       1
#define DBG_CONF_USB             1 /* command output uses debug output */
#define LPM_CONF_ENABLE          0 /* serial radio never in low-power mode */

/* Use different product description for USB */
#define USB_SERIAL_RADIO_GET_PRODUCT_DESCRIPTION serial_radio_get_product_description
#define USB_SERIAL_GET_PRODUCT_DESCRIPTION dual_mode_get_product_description

#define STORE_LOCATIONID_LOCALLY 1

#define FRAMER_802154_HANDLER handler_802154_frame_received

/* FIXME check if this is really needed */
#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM        4

#define CMD_CONF_OUTPUT serial_radio_cmd_output
#define CMD_CONF_ERROR  serial_radio_cmd_error

/* A generic RDC driver for IoT-U10 with run-time module selection */
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC dual_mode_rdc_driver

/* A generic NETWORK driver for IoT-U10 with run-time module selection */
#undef NETSTACK_CONF_NETWORK
#define NETSTACK_CONF_NETWORK dual_mode_net_driver

/* A generic FRAMER for IoT-U10 with run-time module selection */
#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER dual_mode_framer

/* Default serial radio watchdog for radio frontpanel in seconds */
#define RADIO_FRONTPANEL_WATCHDOG_RESET_VALUE (60 * 60)

/* Network statistics */
#define RPL_CONF_STATS 1

/* Enable mode selection in run-time */
#define PLATFORM_WITH_DUAL_MODE 1
typedef enum {
  DUAL_MODE_OP_MODE_STANDARD      = 0,
  DUAL_MODE_OP_MODE_SERIAL_RADIO  = 1,
} dual_mode_op_mode_t;
/* Return the current operation mode of a mote. */
dual_mode_op_mode_t dual_mode_get_op_mode(void);

#define PRODUCTTYPE_INT64_SERIAL_RADIO 0x0090DA0301010482ULL
#define PRODUCTTYPE_INT64_STANDARD_MODE 0x0090DA0301010501ULL
/* Default product type for trailer is, now, set to the serial radio */
#define PRODUCT_TYPE_INT64 0x0090DA0301010482ULL
/* product-type is used in run-time only, so the following macro should return the right number. */
#define PRODUCTTYPE_INT64 ((dual_mode_get_op_mode() == DUAL_MODE_OP_MODE_STANDARD) ? (PRODUCTTYPE_INT64_STANDARD_MODE) : (PRODUCTTYPE_INT64_SERIAL_RADIO))

#if 0
#define PRODUCT_LABEL_SERIAL_RADIO  "Serial Radio"
#define PRODUCT_LABEL_STANDARD_MODE "Node"
#define PRODUCT_LABEL ((dual_mode_get_op_mode() == DUAL_MODE_OP_MODE_STANDARD) ? (PRODUCT_LABEL_STANDARD_MODE) : (PRODUCT_LABEL_SERIAL_RADIO))
#else
#define PRODUCT_LABEL "Sparrow Dual-Op"
#endif

#define SERIAL_RADIO_CONTROL_API_VERSION 3L

#endif /* PROJECT_CONF_H_ */
