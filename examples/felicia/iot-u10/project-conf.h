/*
 * Copyright (c) 2015-2016, Yanzi Networks AB.
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
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define PRODUCT_TYPE_INT64 0x0090DA0301010501ULL
#define PRODUCT_LABEL "IoT-U10"

#ifdef BOARD_STRING
#define LWM2M_DEVICE_MODEL_NUMBER BOARD_STRING
#else
#define LWM2M_DEVICE_MODEL_NUMBER PRODUCT_LABEL
#define LWM2M_DEVICE_MANUFACTURER "Yanzi Networks AB."
#define LWM2M_DEVICE_SERIAL_NO    "90DA0301010501"
#endif

#define IPSO_TEMPERATURE platform_ipso_temperature

#define INSTANCE_TEMPERATURE_ARCH_INIT stts751_millikelvin
uint32_t INSTANCE_TEMPERATURE_ARCH_INIT(void);

#define INSTANCE_TEMPERATURE_ARCH_READVAL stts751_millikelvin
#define INSTANCE_TEMPERATURE_ARCH_ERRORVAL stts751_errorval

#define RESOURCE_LED0_CONF_LED    LEDS_YELLOW
#define RESOURCE_LED1_CONF_LED    LEDS_GREEN

#define LEDS_CONTROL_CONF_NUMBER	2

#define USB_SERIAL_CONF_ENABLE 1
#define DBG_CONF_USB 1

/* Network statistics */
#define RPL_CONF_STATS 1
/* #define HANDLER_802154_CONF_STATS 1 */

/* #define RPL_CALLBACK_PARENT_SWITCH \ */
/*   instance_nstats_preferred_parent_callback */

/* CoAP */
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE 256

#define WEBSERVER_CONF_CFS_PATHLEN 24

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


#endif /* PROJECT_CONF_H_ */
