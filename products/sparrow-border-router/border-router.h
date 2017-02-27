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

/**
 * \file
 *         Border router header file
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 */

#ifndef BORDER_ROUTER_H_
#define BORDER_ROUTER_H_

#include "contiki.h"
#include "net/ip/uip.h"
#include "dev/radio.h"

#define GLOBAL_CHASSIS_CAPABILITY_MASK          0x8000000000000000ULL
#define GLOBAL_CHASSIS_CAPABILITY_CAN_SLEEP     0x01ULL
#define GLOBAL_CHASSIS_CAPABILITY_CTS_RTS       0x02ULL
#define GLOBAL_CHASSIS_CAPABILITY_MISSING_FAN   0x04ULL

#define VARIABLE_ROUTER_READY 0x106

typedef enum {
  BORDER_ROUTER_STATUS_CONFIGURATION_READY = 1,
  BORDER_ROUTER_STATUS_CONFIGURATION_HAS_STATUS_BITS = 2,
  BORDER_ROUTER_STATUS_CONFIGURATION_PAN_ID = 4,
  BORDER_ROUTER_STATUS_CONFIGURATION_BEACON = 8,
  BORDER_ROUTER_STATUS_CONFIGURATION_MODE = 16,
} border_router_status_configuration_t;

#ifdef BORDER_ROUTER_CONF_SET_RADIO_WATCHDOG
#define BORDER_ROUTER_SET_RADIO_WATCHDOG BORDER_ROUTER_CONF_SET_RADIO_WATCHDOG
#else
#define BORDER_ROUTER_SET_RADIO_WATCHDOG 0
#endif

#define RADIO_INFO_OBJECT_TYPE   0x01
#define RADIO_INFO_OBJECT_ID     0x02
#define RADIO_INFO_INSTANCES     0x04
#define RADIO_INFO_SW_REVISION   0x08
#define RADIO_INFO_BL_VERSION    0x10
#define RADIO_INFO_CAPABILITIES  0x20
#define RADIO_INFO_NEEDED_INFO   0x3f /* what is needed to start OAM  */

#define RADIO_MODE_IDLE          0x00
#define RADIO_MODE_NORMAL        0x01

int border_router_is_slave(void);
void border_router_set_slave(int is_slave);
int border_router_is_slave_active(void);

int border_router_cmd_handler(const uint8_t *data, int len);
int br_config_handle_arguments(int argc, char **argv);
void write_to_slip(const uint8_t *buf, int len);
void write_to_slip_payload_type(const uint8_t *buf, int len, uint8_t payload_type);

void border_router_reset_radio(void);
void border_router_set_mac(const uint8_t *data);
void border_router_set_radio_watchdog(uint16_t seconds);
void border_router_set_radio_mode(uint8_t mode);
uint8_t border_router_get_radio_mode(void);
void border_router_set_frontpanel_info(uint16_t info);
void border_router_set_panid(uint16_t pan_id);
void border_router_request_radio_version(void);
void border_router_request_radio_time(void);
void border_router_print_stat(void);

void border_router_set_ctsrts(uint32_t ctsrts);
void border_router_set_baudrate(unsigned speed, int testonly);

void border_router_radio_set_value(radio_param_t param, radio_value_t value);

void border_router_ctrl_init(void);
void border_router_server_init(void);
int border_router_server_has_client_connection(void);
uint16_t border_router_server_get_port(void);

void tun_init(void);

#define SERIAL_MODE_ACK  (1 << 0)
#define SERIAL_MODE_NACK (1 << 1)

uint32_t serial_get_mode(void);
void serial_set_mode(uint32_t mode);

uint32_t serial_get_send_delay(void);
void serial_set_send_delay(uint32_t delayms);

unsigned serial_get_baudrate(void);
void serial_set_baudrate(unsigned speed);

int serial_get_ctsrts(void);
void serial_set_ctsrts(int ctsrts);

#endif /* BORDER_ROUTER_H_ */
