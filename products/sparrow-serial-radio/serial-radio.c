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
 *         Serial-radio driver
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 */

#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/slip.h"
#include "dev/watchdog.h"
#include "dev/sparrow-device.h"
#include "net/netstack.h"
#include "net/packetbuf.h"

#include "cmd.h"
#include "serial-radio.h"
#include "sniffer-rdc.h"
#include "radio-scan.h"
#include "packetutils.h"
#include "serial-radio-stats.h"
#include "radio-frontpanel.h"
#include "transmit-buffer.h"
#include "sparrow-oam.h"
#include "enc-net.h"
#include <string.h>
#include <inttypes.h>

#ifndef SERIAL_RADIO_CONTROL_API_VERSION
#error "No SERIAL_RADIO_CONTROL_API_VERSION specified. Please set in project-conf.h!"
#endif

extern char *sparrow_radio_name;

#ifdef CONTIKI_BOARD_IOT_U42
extern const struct radio_driver cc2538_rf_driver;
extern const struct radio_driver cc1200_driver;
#endif /* CONTIKI_BOARD_IOT_U42 */

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define SNIFFER_MODE_NORMAL 0
#define SNIFFER_MODE_ATTS   1

#define BOOT_CMD "!!SERIAL RADIO BOOTED"

uint32_t border_router_api_version = 1;

#ifdef HAVE_SERIAL_RADIO_UART
#include "dev/uart1.h"

#define TEST_CTS_RTS  (1 << 0)
#define TEST_BAUDRATE (1 << 1)
#define TEST_PERIOD   (30 * CLOCK_SECOND)
static struct ctimer test_timer;
static uint8_t active_test;

extern unsigned int uart1_framerr;
extern unsigned int uart1_parerr;
extern unsigned int uart1_overrunerr;
extern unsigned int uart1_timeout;
#endif /* HAVE_SERIAL_RADIO_UART */


/* Current mode - default from boot is idle mode */
radio_mode_t serial_radio_mode = RADIO_MODE_IDLE;
static uint8_t active_channel;
static uint8_t sniffer_mode = SNIFFER_MODE_NORMAL;

/* max 32 packets at the same time??? */
static uint8_t packet_ids[32];
static int packet_pos;

static int serial_radio_cmd_handler(const uint8_t *data, int len);

uint8_t verbose_output = 1;

/*---------------------------------------------------------------------------*/
/* Debug serial transmissions */
static struct ctimer debug_timer;
static uint16_t debug_count, debug_size;
static void
debug_sender(void *ptr)
{
  static uint16_t seqno;
  int i;
  uint8_t buf[256];
  if(debug_count == 0) {
    return;
  }
  debug_count--;
  if(debug_count > 0) {
    ctimer_restart(&debug_timer);
  }
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = 'S';
  seqno++;
  buf[3] = (seqno >> 8) & 0xff;
  buf[4] = seqno & 0xff;
  if(debug_size > sizeof(buf)) {
    debug_size = sizeof(buf);
  }
  for(i = 5; i < debug_size; i++) {
    buf[i] = i - 5;
  }
  cmd_send(buf, debug_size);
  if(debug_count == 0) {
    clock_delay_usec(2000);
    printf("Finished sending debug data\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
set_radio_address_filter(int onoroff)
{
  radio_value_t rxmode;

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_RX_MODE, &rxmode) != RADIO_RESULT_OK) {
    rxmode = 0;
  }

  if(onoroff) {
    rxmode |= RADIO_RX_MODE_ADDRESS_FILTER | RADIO_RX_MODE_AUTOACK;
  } else {
    rxmode &= ~((radio_value_t)(RADIO_RX_MODE_ADDRESS_FILTER | RADIO_RX_MODE_AUTOACK));
  }
  NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, rxmode);
}
/*---------------------------------------------------------------------------*/
#ifdef HAVE_SERIAL_RADIO_UART
static void
configure_cts_rts(int onoroff)
{
  uart1_flow_enable(onoroff);
}
#endif /* HAVE_SERIAL_RADIO_UART */
/*---------------------------------------------------------------------------*/
#ifdef HAVE_SERIAL_RADIO_UART
static void
test_callback(void *ptr)
{
  if(active_test & TEST_CTS_RTS) {
    /* Disable CTS/RTS */
    active_test &= ~TEST_CTS_RTS;
    configure_cts_rts(0);
    clock_delay_usec(50);
    printf("End of test CTS/RTS\n");
  }
  if(active_test & TEST_BAUDRATE) {
    /* Revert to default baudrate */
    active_test &= ~TEST_BAUDRATE;
    uart1_init(SERIAL_BAUDRATE);
    clock_delay_usec(50);
    printf("End of test baudrate\n");
  }
}
#endif /* HAVE_SERIAL_RADIO_UART */
/*---------------------------------------------------------------------------*/
#if defined(CONTIKI_TARGET_FELICIA) || (PLATFORM_WITH_DUAL_MODE) || (defined(CONTIKI_TARGET_ZOUL_SPARROW) && defined(WITH_USB_PORT))
/* Use different product name for USB on platform Felicia */
struct product {
  uint8_t size;
  uint8_t type;
  uint16_t string[20];
};
static const struct product product = {
  sizeof(product),
  3,
  {
    'S','p','a','r','r','o','w',' ','S','e','r','i','a','l',' ','R','a','d','i','o'
  }
};
uint8_t *
serial_radio_get_product_description(void)
{
  return (uint8_t *)&product;
}
#endif /* defined(CONTIKI_TARGET_FELICIA) || (PLATFORM_WITH_DUAL_MODE)*/
/*---------------------------------------------------------------------------*/
static sniffer_rdc_filter_op_t
sniffer_callback(void)
{
  if(sniffer_mode == SNIFFER_MODE_ATTS) {
    /*
     * Use the normal way to send up the received packet including
     * packet attributes.
     */
    return SNIFFER_RDC_FILTER_PROCESS;
  }

  if(packetbuf_hdralloc(2)) {
    uint8_t *p;
    p = packetbuf_hdrptr();
    p[0] = '!';
    p[1] = 'h';
    cmd_send(p, packetbuf_totlen());
  } else {
    /* packetbuf + hdr should always be sufficient large. */
    PRINTF("radio: too much packet data from sniffer\n");
  }
  return SNIFFER_RDC_FILTER_DROP;
}
/*---------------------------------------------------------------------------*/
radio_mode_t
radio_get_mode(void)
{
  return serial_radio_mode;
}
/*---------------------------------------------------------------------------*/
void
radio_set_mode(radio_mode_t mode)
{
  if(serial_radio_mode == mode) {
    /* no change */
    return;
  }

  /* Actions when leaving a radio mode */
  switch(serial_radio_mode) {
  case RADIO_MODE_IDLE:
  case RADIO_MODE_NORMAL:
    break;
  case RADIO_MODE_SNIFFER:
    set_radio_address_filter(1);
    break;
  case RADIO_MODE_SCAN:
    radio_scan_stop();
    /* Restore the active channel */
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, active_channel);
    break;
  }

  serial_radio_mode = mode;

  /* Actions when entering a radio mode */
  switch(mode) {
  case RADIO_MODE_IDLE:
  case RADIO_MODE_NORMAL:
    break;
  case RADIO_MODE_SNIFFER:
    sniffer_rdc_set_sniffer(sniffer_callback);
    set_radio_address_filter(0);
    break;
  case RADIO_MODE_SCAN:
    /* Activate the scanning */
    radio_scan_start();
    break;
  }
}
/*---------------------------------------------------------------------------*/
void
serial_radio_set_pan_id(uint16_t pan_id, uint8_t notify)
{
  uint8_t buf[5];

  printf("radio: setting pan id: %u (0x%04x)\n", pan_id, pan_id);
  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, pan_id);

  if(notify) {
    /* Tell our border router */
    buf[0] = '!';
    buf[1] = 'P';
    buf[2] = pan_id >> 8;
    buf[3] = pan_id & 0xff;
    cmd_send(buf, 4);
  }
}
/*---------------------------------------------------------------------------*/
unsigned
serial_radio_get_channel(void)
{
  return active_channel;
}
/*---------------------------------------------------------------------------*/
void
serial_radio_set_channel(unsigned channel)
{
  active_channel = channel & 0xff;
  printf("radio: setting channel: %u\n", active_channel);
  if(serial_radio_mode == RADIO_MODE_SCAN) {
    /* Notify the radio scanner about the channel change */
    radio_scan_set_channel(active_channel);
  } else {
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, active_channel);
  }
}
/*---------------------------------------------------------------------------*/
void
serial_radio_send_watchdog_warning(uint16_t seconds_left)
{
  uint8_t buf[8];
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = 'W';
  buf[3] = (seconds_left >> 8) & 0xff;
  buf[4] = seconds_left & 0xff;
  cmd_send(buf, 5);
}
/*---------------------------------------------------------------------------*/
#ifdef HAVE_RADIO_FRONTPANEL
static const char *
get_supply_type_name(uint8_t type)
{
  switch(type) {
  case RADIO_SUPPLY_TYPE_UNKNOWN:
    return "unknown";
  case RADIO_SUPPLY_TYPE_GRID:
    return "grid";
  case RADIO_SUPPLY_TYPE_UPS:
    return "UPS";
  case RADIO_SUPPLY_TYPE_BATTERY:
    return "battery";
  default:
    return "unknown";
  }
}
#endif /* HAVE_RADIO_FRONTPANEL */
/*---------------------------------------------------------------------------*/
#ifdef HAVE_RADIO_FRONTPANEL
static const char *
get_supply_status_name(uint8_t status)
{
  switch(status) {
  case RADIO_SUPPLY_STATUS_UNKNOWN:
    return "unknown";
  case RADIO_SUPPLY_STATUS_USING_GRID:
    return "using grid";
  case RADIO_SUPPLY_STATUS_USING_GRID_AND_CHARGING_UPS:
    return "using grid and charging UPS";
  case RADIO_SUPPLY_STATUS_USING_UPS:
    return "using UPS";
  case RADIO_SUPPLY_STATUS_USING_BATTERY:
    return "using battery";
  default:
    return "unknown";
  }
}
#endif /* HAVE_RADIO_FRONTPANEL */
/*---------------------------------------------------------------------------*/
#ifdef CMD_CONF_HANDLERS
CMD_HANDLERS(serial_radio_cmd_handler, CMD_CONF_HANDLERS);
#else
CMD_HANDLERS(serial_radio_cmd_handler);
#endif
/*---------------------------------------------------------------------------*/
static void
packet_sent(void *ptr, int status, int transmissions)
{
  uint8_t buf[20];
  uint8_t sid;
  int pos;
  sid = *((uint8_t *)ptr);
  if(verbose_output > 1) {
    PRINTF("radio: packet sent! sid: %d, status: %d, tx: %d\n",
           sid, status, transmissions);
  }
  /* packet callback from lower layers */
  /*  neighbor_info_packet_sent(status, transmissions); */
  pos = 0;
  buf[pos++] = '!';
  buf[pos++] = 'R';
  buf[pos++] = sid;
  buf[pos++] = status; /* one byte ? */
  buf[pos++] = transmissions;
  cmd_send(buf, pos);
}
/*---------------------------------------------------------------------------*/
static void
send_version(int radio_restarted)
{
  uint8_t buf[6];
  buf[0] = '!';
  buf[1] = radio_restarted ? '!' : 'v';
  buf[2] = (SERIAL_RADIO_CONTROL_API_VERSION >> 24) & 0xff;
  buf[3] = (SERIAL_RADIO_CONTROL_API_VERSION >> 16) & 0xff;
  buf[4] = (SERIAL_RADIO_CONTROL_API_VERSION >>  8) & 0xff;
  buf[5] = (SERIAL_RADIO_CONTROL_API_VERSION >>  0) & 0xff;
  cmd_send(buf, 6);
}
/*---------------------------------------------------------------------------*/
static void
send_clock(const uint8_t *intime)
{
  uint8_t buf[2 + 8 + 8];
  int i;
  uint64_t now;
  now = uptime_read();
  buf[0] = '!';
  buf[1] = 't';
  for(i = 0; i < 8; i++) {
    buf[2 + i] = intime[i];
    buf[8 + 9 - i] = now & 0xff;
    now = now >> 8;
  }
  cmd_send(buf, 2 + 8 + 8);
}
/*---------------------------------------------------------------------------*/
/* Packet in packet-buf - just needs parsing for addresses and then send */
void
serial_radio_send_packet(uint8_t id)
{
  /* parse frame before sending to get addresses, etc. */
  packet_ids[packet_pos] = id;

  NETSTACK_FRAMER.parse();
  NETSTACK_MAC.send(packet_sent, &packet_ids[packet_pos]);

  packet_pos++;
  if(packet_pos >= sizeof(packet_ids)) {
    packet_pos = 0;
  }
}
/*---------------------------------------------------------------------------*/
static int
serial_radio_cmd_handler(const uint8_t *data, int len)
{
  int i;

  if(data[0] == '!') {
    /* should send out stuff to the radio - ignore it as IP */
    /* --- s e n d --- */
    switch(data[1]) {
    case 'S':
    case 'Z': {

      if(packetutils_deserialize_packetbuf(&data[3], len - 3) <= 0) {
        PRINTF("radio: illegal packet format\n");
        return 1;
      }

      if(data[1] == 'S') {
        if(verbose_output > 1) {
          PRINTF("radio: sending %u (%d bytes)\n",
                 data[2], packetbuf_datalen());
        }
        serial_radio_send_packet(data[2]);
      } else {
        clock_time_t time;
        uint16_t time16;
        uint16_t low_time;
        uint16_t diff;

        time16 = packetbuf_attr(PACKETBUF_ATTR_TRANSMIT_TIME);
        time = clock_time();
        low_time = time & 0xffff;
        diff = time16 - low_time;
        /* if the scheduled time minus low part of timer is less than
           half a 16-bit value we are probably ok from a wrap */
        PRINTF("SR: delayed transm: tgt:%u low:%u diff:%d\n", time16, low_time, diff);
        if(diff < 0x8000) {
          time = time + diff;
          if(verbose_output > 1) {
            PRINTF("radio: storing delayed transmission: %u (%d bytes) send in:%u\n", data[2], packetbuf_datalen(), diff);
          }
          if(!transmit_buffer_add_packet(time, data[2])) {
            PRINTF("radio: failed to store delayed transmission\n");
            serial_radio_send_packet(data[2]);
          }
        } else {
          /* we have probably missed the time??? */
          PRINTF("radio: failed delayed transmission: %u (%d bytes) send in:%u - sending now!\n", data[2], packetbuf_datalen(), diff);
          serial_radio_send_packet(data[2]);
        }
      }
      return 1;
    }
    case 's': {
      /* Send directly to radio */
      if(serial_radio_mode == RADIO_MODE_NORMAL) {
        if(verbose_output > 1) {
          PRINTF("radio: direct sending %d bytes\n", len);
        }
        packetbuf_clear();
        NETSTACK_RADIO.send(&data[2], len - 2);
      }
      return 1;
    }
    case 'i':
      /* Select radio if multiple radios are supported */
      return 1;
    case 'C': {
      serial_radio_set_channel(data[2]);
      return 1;
    }
    case 'T': {
      int8_t txpower = data[2];
      printf("radio: setting tx power: %d dBm\n", txpower);
      NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, txpower);
      return 1;
    }
    case 'P': {
      uint16_t pan_id = ((uint16_t)data[2] << 8) | data[3];
      serial_radio_set_pan_id(pan_id, 0);
      return 1;
    }
    case 'V': {
      int type = ((uint16_t)data[2] << 8) | data[3];
      int value = ((uint16_t)data[4] << 8) | data[5];
      int param = packetutils_to_radio_param(type);
      if(param < 0) {
        printf("radio: unknown parameter %d (can not set to %d)\n", type, value);
      } else {
        if(param == RADIO_PARAM_CHANNEL) {
          /**
           * Special handling of radio channel since the radio might
           * be in scan mode.
           */
          serial_radio_set_channel(value);
        } else {
          if(param == RADIO_PARAM_RX_MODE) {
            printf("radio: setting rxmode to 0x%x\n", value);
          } else if(param == RADIO_PARAM_PAN_ID) {
            printf("radio: setting pan id to 0x%04x\n", value);
          } else {
            printf("radio: setting param %d to %d (0x%02x)\n", param, value, value);
          }
          NETSTACK_RADIO.set_value(param, value);
        }
      }
      return 1;
    }
    case 'm': {
      if(serial_radio_mode != data[2]) {
        radio_set_mode(data[2]);
        printf("radio: mode set to %u\n", serial_radio_mode);
      } else {
        printf("radio: already in mode %u\n", serial_radio_mode);
      }
      return 1;
    }
    case 'c': {
      radio_scan_set_mode(data[2]);
      printf("radio scan: mode set to %u\n", data[2]);
      return 1;
    }
    case 'f': {
      /* Radio ext mode */
      if(data[2] == 's') {
        /* Sniffer mode */
        sniffer_mode = data[3];
        printf("radio sniffer mode set to %u\n", sniffer_mode);
      } else {
        printf("unknown format 'f%c'\n", data[2]);
      }
      return 1;
    }
    case 'd': {
      verbose_output = data[2];
      printf("verbose output is set to %u\n", verbose_output);
      return 1;
    }
    case 'x': {
      unsigned long rate;
      switch(data[2]) {
      case '0':
        /* Reset radio to inital values */
        radio_set_mode(RADIO_MODE_IDLE);
        sniffer_mode = SNIFFER_MODE_NORMAL;
        set_radio_address_filter(1);
        radio_scan_set_mode(RADIO_SCAN_MODE_NORMAL);
        break;
#ifdef HAVE_SERIAL_RADIO_UART
      case 'c':
        active_test &= ~TEST_CTS_RTS;
        if(data[3] == 2) {
          /* Enable CTS/RTS */
          printf("Enabling CTS/RTS\n");
          clock_delay_usec(2000);
          configure_cts_rts(1);
        } else if(data[3] == 1) {
          /* Temporary test CTS/RTS */
          printf("Testing CTS/RTS\n");
          clock_delay_usec(2000);
          configure_cts_rts(1);
          active_test |= TEST_CTS_RTS;
          ctimer_set(&test_timer, TEST_PERIOD, test_callback, NULL);
        } else {
          /* Disable CTS/RTS */
          configure_cts_rts(0);
          printf("Disabling CTS/RTS\n");
        }
        return 1;
      case 'u':
      case 'U':
        rate = (data[3] << 24) + (data[4] << 16) + (data[5] << 8) + data[6];
        if(rate < 1200) {
          printf("Illegal baudrate: %lu\n", rate);
          return 1;
        }
        if(data[2] == 'U') {
          printf("Testing baudrate %lu\n", rate);
          active_test |= TEST_BAUDRATE;
          ctimer_set(&test_timer, TEST_PERIOD, test_callback, NULL);
        } else {
          printf("Changing baudrate to %lu\n", rate);
          active_test &= ~TEST_BAUDRATE;
        }
        clock_delay_usec(2000);
        uart1_init(rate);
        return 1;
#endif /* HAVE_SERIAL_RADIO_UART  */
#ifdef HAVE_RADIO_FRONTPANEL
      case 'e':
        rate = (data[3] << 8) + data[4];
        radio_frontpanel_set_error_code(rate);
        printf("Set frontpanel error code to %lu\n", rate);
        return 1;
      case 'l':
        rate = (data[3] << 8) + data[4];
        radio_frontpanel_set_info(rate);
        printf("Set frontpanel info to %lu\n", rate);
        return 1;
#endif /* HAVE_RADIO_FRONTPANEL */
      case 'd':
        rate = (data[3] << 8) + data[4];
        enc_net_set_fragment_delay(rate & 0xffff);
        printf("Changing fragment delay to %lu\n", rate);
        return 1;
      case 'b':
        rate = (data[3] << 8) + data[4];
        enc_net_set_fragment_size(rate & 0xffff);
        printf("Changing fragment size to %lu\n", rate);
        return 1;
      case 'r':
        printf("Rebooting node!\n");
        clock_delay_usec(1000);
        if(len > 2) {
          SPARROW_DEVICE.reboot_to_image(data[3]);
        } else {
          SPARROW_DEVICE.reboot();
        }
        return 1;
#ifdef HAVE_RADIO_FRONTPANEL
      case 'F':
        rate = (data[3] << 8) + data[4];
        radio_frontpanel_set_fan_speed(rate);
        printf("Set fan speed to 0x%lx\n", rate);
        return 1;
      case 'W':
        rate = (data[3] << 8) + data[4];
        radio_frontpanel_set_watchdog(rate);
        PRINTF("Set radio watchdog to trig in %lu sec\n", rate);
        return 1;
#endif /* HAVE_RADIO_FRONTPANEL */
      case 's':
        debug_count = (data[3] << 8) + data[4];
        debug_size = (data[5] << 8) + data[6];
        printf("Prepared to send %u debug data of size %u\n",
               debug_count, debug_size);
        ctimer_set(&debug_timer, (data[7] << 8) + data[8], debug_sender, NULL);
        return 1;
      }
      return 1;
    }
    }
  } else if(data[0] == '?') {
    radio_value_t value;

    if(verbose_output > 1) {
      PRINTF("radio: got request message of type %c\n", data[1]);
    }
    switch(data[1]) {
    case 'M': {
      uint8_t buf[10];
      buf[0] = '!';
      buf[1] = 'M';
      for(i = 0; i < 8; i++) {
        buf[2 + i] = uip_lladdr.addr[i];
      }
      cmd_send(buf, 10);
      return 1;
    }
    case 'C': {
      uint8_t buf[4];
      /* buf[2] = active_channel; */
      /* Return the channel currently being used by the radio */
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &value) == RADIO_RESULT_OK) {
        buf[0] = '!';
        buf[1] = 'C';
        buf[2] = value & 0xff;
        cmd_send(buf, 3);
      } else {
        PRINTF("Failed to get channel\n");
      }
      return 1;
    }
    case 'T': {
      uint8_t buf[4];
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &value) == RADIO_RESULT_OK) {
        buf[0] = '!';
        buf[1] = 'T';
        buf[2] = value & 0xff;
        cmd_send(buf, 3);
      } else {
        PRINTF("Failed to get txpower\n");
      }
      return 1;
    }
    case 'i': {
      /* Return which radio is active if multiple radios are supported */
      uint8_t buf[4];
      buf[0] = '!';
      buf[1] = 'i';
      buf[2] = 0;
      buf[3] = 0;
      cmd_send(buf, 4);
      return 1;
    }
    case 't': {
      /* send back the same 8 bytes time + SR now */
      send_clock(&data[2]);
      return 1;
    }
    case 'P': {
      uint8_t buf[5];
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_PAN_ID, &value) == RADIO_RESULT_OK) {
        buf[0] = '!';
        buf[1] = 'P';
        buf[2] = (value >> 8) & 0xff;
        buf[3] = value & 0xff;
        cmd_send(buf, 4);
      } else {
        PRINTF("failed to get pan id\n");
      }
      return 1;
    }
    case 'V': {
      int type = ((uint16_t)data[2] << 8) | data[3];
      int param = packetutils_to_radio_param(type);
      uint8_t buf[7];

      if(param < 0) {
        printf("radio: unknown parameter %d\n", type);
      } else if(NETSTACK_RADIO.get_value(param, &value) == RADIO_RESULT_OK) {
        buf[0] = '!';
        buf[1] = 'V';
        buf[2] = (type >> 8) & 0xff;
        buf[3] = type & 0xff;
        buf[4] = (value >> 8) & 0xff;
        buf[5] = value & 0xff;
        cmd_send(buf, 6);
      } else {
        printf("radio: could not get value for %d\n", param);
      }
      return 1;
    }
    case 'm': {
      uint8_t buf[3];
      buf[0] = '!';
      buf[1] = 'm';
      buf[2] = serial_radio_mode;
      cmd_send(buf, 3);
      return 1;
    }
    case 'c': {
      uint8_t buf[3];
      buf[0] = '!';
      buf[1] = 'c';
      buf[2] = radio_scan_get_mode();
      cmd_send(buf, 3);
      return 1;
    }
    case 'f': {
      /* Return the radio ext mode */
      uint8_t buf[5];
      buf[0] = '!';
      buf[1] = 'f';
      buf[2] = data[2];
      if(data[2] == 's') {
        buf[3] = sniffer_mode;
      } else {
        buf[3] = 0;
      }
      cmd_send(buf, 4);
      return 1;
    }
    case 'v': {
      if(len > 2) {
        border_router_api_version =
          (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5];
      }
      printf("Got BR API version:%"PRIu32"\n", border_router_api_version);
      send_version(0);
      return 1;
    }
    case 'd': {
      uint8_t buf[3];
      buf[0] = '!';
      buf[1] = 'd';
      buf[2] = verbose_output;
      cmd_send(buf, 3);
      return 1;
    }
    case 'S':
      /* Status */
      printf("ENCAP: received %lu bytes payload, %lu errors\n",
             SERIAL_RADIO_STATS_GET(SERIAL_RADIO_STATS_ENCAP_RECV),
             SERIAL_RADIO_STATS_GET(SERIAL_RADIO_STATS_ENCAP_ERRORS));
      printf("SERIAL: received %lu bytes, %lu frames\n",
             SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_RECV),
             SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_FRAMES));
      printf("SLIP: %lu overflows, %lu dropped bytes, %lu errors\n",
             SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_OVERFLOWS),
             SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_DROPPED),
             SERIAL_RADIO_STATS_DEBUG_GET(SERIAL_RADIO_STATS_DEBUG_SLIP_ERRORS));
#ifdef HAVE_SERIAL_RADIO_UART
      /* Only show UART statistics if at least one error has occurred */
      if(uart1_framerr || uart1_parerr || uart1_overrunerr || uart1_timeout) {
        printf("UART1: f=%u p=%u o=%u timeout=%u\n",
               uart1_framerr, uart1_parerr, uart1_overrunerr, uart1_timeout);
      }
#endif /* HAVE_SERIAL_RADIO_UART */

      if(sparrow_radio_name != NULL) {
        printf("PHY: %s\n", sparrow_radio_name);
      } else {
        printf("PHY: unknown\n");
      }

#ifdef CONTIKI_BOARD_IOT_U42
      {
        const char *name;
        if(&NETSTACK_RADIO == &cc2538_rf_driver) {
          name = "CC2538 RF";
        } else if(&NETSTACK_RADIO == &cc1200_driver) {
          name = "CC1200";
        } else {
          name = "unknown";
        }
        printf("PHY: %s\n", name);
      }
#endif /* CONTIKI_BOARD_IOT_U42 */
#ifdef HAVE_RADIO_FRONTPANEL
      {
        uint32_t r = radio_get_reset_cause();
        uint32_t wd = radio_frontpanel_get_watchdog();
        uint32_t s = radio_frontpanel_get_supply_status();

        printf("Frontpanel: info %"PRIu32", reset 0x%02x, ext reset 0x%02x\n",
               radio_frontpanel_get_info(),
               (unsigned)((r >> 8) & 0xff), (unsigned)(r & 0xff));
        printf("Frontpanel: supply type %s status %s, on battery %"PRIu32" seconds, %"PRIu32" mV (charge pulses %u)\n",
               get_supply_type_name((s >> 8) & 0xff),
               get_supply_status_name(s & 0xff),
               radio_frontpanel_get_battery_supply_time(),
               radio_frontpanel_get_supply_voltage(),
               radio_frontpanel_get_charger_pulse_count());
        if(wd == 0) {
          printf("Frontpanel: Radio watchdog is disabled\n");
        } else {
          printf("Frontpanel: Radio watchdog will expire in %"PRIu32" seconds\n", wd);
        }
      }
#endif /* HAVE_RADIO_FRONTPANEL */
      return 1;
    case 'x': {
      switch(data[2]) {
#ifdef HAVE_RADIO_FRONTPANEL
      case 'F': {
        uint32_t speed = radio_frontpanel_get_fan_speed();
        printf("Fan is %s (speed is set to 0x%"PRIx32")\n",
               speed ? "on" : "off", speed);
        return 1;
      }
      case 'W': {
        uint32_t wd = radio_frontpanel_get_watchdog();
        if(wd == 0) {
          printf("Radio watchdog is disabled\n");
        } else {
          printf("Radio watchdog will expire in %"PRIu32" seconds\n", wd);
        }
        return 1;
      }
      case 'x':
#ifdef CONTIKI_BOARD_SKIPPER
        if(data[3] == 'x' && data[4] == 'x') {
          printf("KILLING POWER NOW!!!\n");
          platform_system_power(0);
          return 1;
        }
#endif /* CONTIKI_BOARD_SKIPPER */
#endif /* HAVE_RADIO_FRONTPANEL */
        printf("Unknown x command: %02x\n", data[3]);
        return 1;
      default:
        return 1;
      }
    }
    }
  } /* if(data[0] == '?' */
  return 0;
}
/*---------------------------------------------------------------------------*/
void
serial_radio_cmd_output(const uint8_t *data, int data_len)
{
  enc_net_send_packet(data, data_len);
}
/*---------------------------------------------------------------------------*/
void
serial_radio_cmd_error(cmd_error_t error, const uint8_t *data, int data_len)
{
  if(error == CMD_ERROR_UNKNOWN_COMMAND) {
    uint8_t buf[4];
    /* This packet was not accepted by any command handler */
    buf[0] = '!';
    buf[1] = 'E';
    buf[2] = *data;
    buf[3] = *(data + 1);
    enc_net_send_packet(buf, 4);
  } else {
    printf("Command error: %d\n", error);
  }
}
/*---------------------------------------------------------------------------*/
void
serial_radio_input(sparrow_encap_pdu_info_t *pinfo, uint8_t *payload, int enclen)
{
  size_t len, l;
  uint8_t txbuf[1280];

  len = pinfo->payload_len;

  switch(pinfo->payload_type) {

  case SPARROW_ENCAP_PAYLOAD_TLV:
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_TLV);

    l = sparrow_oam_process_request(payload + enclen, len, txbuf, sizeof(txbuf) - 2, pinfo);
    txbuf[l++] = 0;
    txbuf[l++] = 0;
    enc_net_send_packet_payload_type(txbuf, l, pinfo->payload_type);
    break;

  case SPARROW_ENCAP_PAYLOAD_SERIAL:
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_SERIAL);
    cmd_input(payload + enclen, len);
    break;

  default:
    printf("Unprocessed payload type %d\n", pinfo->payload_type);
    SERIAL_RADIO_STATS_INC(SERIAL_RADIO_STATS_ENCAP_UNPROCESSED);
    break;
  } /* switch(payload_type) */
}
/*---------------------------------------------------------------------------*/
PROCESS(serial_radio_process, "Serial radio process");
AUTOSTART_PROCESSES(&serial_radio_process);
/*---------------------------------------------------------------------------*/
static void
init_config(void)
{
  radio_value_t value;
  if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &value) == RADIO_RESULT_OK) {
    active_channel = value;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(serial_radio_process, ev, data)
{
  PROCESS_BEGIN();

  packet_pos = 0;
  init_config();

  NETSTACK_RDC.off(1);

  printf("Serial Radio started...\n");

#ifdef HAVE_RADIO_FRONTPANEL
  radio_frontpanel_init();
#endif /* HAVE_RADIO_FRONTPANEL */

  /* Notify border router that the serial radio has started (or rebooted) */
  cmd_send((uint8_t *)BOOT_CMD, sizeof(BOOT_CMD));

  /* Idle mode from start - do not communicate */
  radio_set_mode(RADIO_MODE_IDLE);

  while(1) {
    PROCESS_YIELD();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
