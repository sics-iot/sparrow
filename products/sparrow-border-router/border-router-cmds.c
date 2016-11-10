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
 *         Sets up some commands for the border router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "cmd.h"
#include "border-router.h"
#include "br-config.h"
#include "border-router-rdc.h"
#include "border-router-cmds.h"
#include "instance-brm.h"
#include "latency-stats.h"
#include "dev/serial-line.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/mac/handler-802154.h"
#include "net/mac/framer-802154.h"
#include "net/net-control.h"
#include "sparrow-oam.h"
#include "enc-dev.h"
#include <string.h>
#include <stdlib.h>
#include "net/packetbuf.h"
#include "packetutils.h"

#define YLOG_LEVEL YLOG_LEVEL_INFO
#define YLOG_NAME  "br-cmd"
#include "ylog.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#ifndef BORDER_ROUTER_CONTROL_API_VERSION
#error "No BORDER_ROUTER_CONTROL_API_VERSION specified. Please set in project-conf.h!"
#endif

#ifndef BORDER_ROUTER_PRINT_RSSI
#define BORDER_ROUTER_PRINT_RSSI 0
#endif

/* initial value of 0 */
uint32_t radio_control_version = 0;

extern uint8_t radio_info;

#define PRINT_TIME_ONCE       (1 << 0)
#define PRINT_TIME_ALWAYS     (1 << 1)
static uint8_t print_flags = 0;

uint8_t command_context;
uint32_t router_status = 0;

void packet_sent(uint8_t sessionid, uint8_t status, uint8_t tx);

/*---------------------------------------------------------------------------*/
PROCESS(border_router_cmd_process, "Border router cmd process");
/*---------------------------------------------------------------------------*/
static net_control_key_t
get_net_control_key(void)
{
  static net_control_key_t net_control_key = NET_CONTROL_KEY_NONE;
  if(net_control_key == NET_CONTROL_KEY_NONE) {
    net_control_key = net_control_alloc_key();
    if(net_control_key == NET_CONTROL_KEY_NONE) {
      printf("Failed to allocate net control key\n");
    }
  }
  return net_control_key;
}
/*---------------------------------------------------------------------------*/
/* The SR time is the exepected time (e.g. diff) in the serial radio */
static clock_time_t sr_time_diff;

/* set min rtt to 100 ms at first... */
static int sr_min_rtt = 100;

clock_time_t
get_sr_time(void)
{
  return clock_time() + sr_time_diff;
}

/* set the serial radio diff */
void
set_sr_time(clock_time_t time)
{
  sr_time_diff = time - clock_time();

  PRINTF("Set SR time diff to: %d  %"CPRIct" =?= %"CPRIct"\n",
         (int) sr_time_diff, get_sr_time(), time);
}
/*---------------------------------------------------------------------------*/

static const uint8_t *
hextoi(const uint8_t *buf, int len, int *v)
{
  *v = 0;
  for(; len > 0; len--, buf++) {
    if(*buf >= '0' && *buf <= '9') {
      *v = (*v << 4) + ((*buf - '0') & 0xf);
    } else if(*buf >= 'a' && *buf <= 'f') {
      *v = (*v << 4) + ((*buf - 'a' + 10) & 0xf);
    } else if(*buf >= 'A' && *buf <= 'F') {
      *v = (*v << 4) + ((*buf - 'A' + 10) & 0xf);
    } else {
      break;
    }
  }
  return buf;
}
/*---------------------------------------------------------------------------*/
static const uint8_t *
dectoi(const uint8_t *buf, int len, int *v)
{
  int negative = 0;
  *v = 0;
  if(len <= 0) {
    return buf;
  }
  if(*buf == '$') {
    return hextoi(buf + 1, len - 1, v);
  }
  if(*buf == '0' && *(buf + 1) == 'x' && len > 2) {
    return hextoi(buf + 2, len - 2, v);
  }
  if(*buf == '-') {
    negative = 1;
    buf++;
  }
  for(; len > 0; len--, buf++) {
    if(*buf < '0' || *buf > '9') {
      break;
    }
    *v = (*v * 10) + ((*buf - '0') & 0xf);
  }
  if(negative) {
    *v = - *v;
  }
  return buf;
}
/*---------------------------------------------------------------------------*/
static int
index_of(uint8_t v, const uint8_t *buf, int offset, int len)
{
  for(; offset < len; offset++) {
    if(buf[offset] == v) {
      return offset;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
static void
printhex(char c)
{
  const char *hex = "0123456789ABCDEF";
  putchar(hex[(c >> 4) & 0xf]);
  putchar(hex[c & 0xf]);
}
/*---------------------------------------------------------------------------*/
static const char *
get_nbr_state_name(uint8_t state)
{
  switch(state) {
  case NBR_INCOMPLETE:
    return "INCOMPLETE";
  case NBR_REACHABLE:
    return "REACHABLE";
  case NBR_STALE:
    return "STALE";
  case NBR_DELAY:
    return "DELAY";
  case NBR_PROBE:
    return "PROBE";
  default:
    return "unknown";
  }
}
/*---------------------------------------------------------------------------*/
int
border_router_cmd_handler(const uint8_t *data, int len)
{
  /* handle global repair, etc here */
  if(data[0] == '!') {
    switch(data[1]) {
    case '!':
      if(command_context == CMD_CONTEXT_RADIO) {
	/* Slip radio has rebooted */
	if(memcmp("SERIAL RADIO BOOTED", data + 2, 19) == 0) {
	  YLOG_INFO("Serial radio has rebooted. Exiting border router.\n");
	  exit(EXIT_FAILURE);
	}
      }
      return 1;
      /* New command for radio packet with attributes - same as !S to SR */
      /* This is from V3 and onwards */
    case 'S':
      if(command_context == CMD_CONTEXT_RADIO) {

	if(packetutils_deserialize_packetbuf(&data[2], len - 2) <= 0) {
	  PRINTF("NBR: illegal packet attributes\n");
	  return 1;
	}

        /* if this packet came with the "S" there should be a timestamp */
        if(br_config_verbose_output > 1) {
          PRINTF("NBR: Packet time in radio: %u (%u)\n",
                 packetbuf_attr(PACKETBUF_ATTR_TIMESTAMP),
                 (unsigned int)(get_sr_time() & 0xffff));
        }

	NETSTACK_RDC.input();
      }
      return 1;
    case 'G':
      if(command_context == CMD_CONTEXT_STDIO) {
        YLOG_INFO("Performing Global Repair...\n");
        rpl_repair_root(RPL_DEFAULT_INSTANCE);
      }
      return 1;
    case 'M':
      if(command_context == CMD_CONTEXT_RADIO) {
        /* We need to know that this is from the serial radio here. */
        border_router_set_mac(&data[2]);
      } else if(command_context == CMD_CONTEXT_STDIO) {
        printf("Changing radio MAC address is not allowed!\n");
      }
      return 1;
    case 'C':
      if(command_context == CMD_CONTEXT_RADIO) {
        /* We need to know that this is from the slip-radio here. */
        YLOG_INFO("Channel is: %d\n", data[2]);
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        int channel = -1;
        dectoi(&data[2], len - 2, &channel);
        if(channel >= 0) {
          uint8_t buf[5];
          buf[0] = '!';
          buf[1] = 'C';
          buf[2] = channel & 0xff;
          write_to_slip(buf, 3);

          border_router_radio_set_value(RADIO_PARAM_CHANNEL, channel);
          return 1;
        }
        YLOG_ERROR("*** illegal channel: %u\n", channel);
        return 1;
      }
      return 1;
    case 'i':
      if(command_context == CMD_CONTEXT_RADIO) {
        YLOG_INFO("Radio %u/%u is active!\n", data[2], data[3]);
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[4];
        int radio;
        dectoi(&data[2], len - 2, &radio);
        buf[0] = '!';
        buf[1] = 'i';
        buf[2] = radio & 0xff;
        write_to_slip(buf, 3);
        return 1;
      }

    case 't':
      if(command_context == CMD_CONTEXT_RADIO) {
#if ((YLOG_LEVEL) & YLOG_LEVEL_DEBUG) == YLOG_LEVEL_DEBUG
        static clock_time_t last_clock;
        static uint64_t     last_time;
#endif /* ((YLOG_LEVEL) & YLOG_LEVEL_DEBUG) == YLOG_LEVEL_DEBUG */
        clock_time_t sr_old_diff;
        uint64_t time;
        uint64_t old_time;
        int rtt;
        int i;
        time = old_time = 0;
        for(i = 0; i < 8; i++) {
          old_time = (old_time << 8) + data[2 + i];
          time = (time << 8) + data[2 + 8 + i];
        }
        rtt = (int)(clock_time() - old_time);
#if ((YLOG_LEVEL) & YLOG_LEVEL_DEBUG) == YLOG_LEVEL_DEBUG
        YLOG_DEBUG("SR time is: %lu rtt: %3d (%3d)  (%8lu / %lu) T Diff %d SR Diff:%d\n",
                   (unsigned long)time,
                   rtt, sr_min_rtt,
                   (unsigned long)clock_time(),
                   (unsigned long)get_sr_time(),
                   (int)(clock_time() - last_clock),
                   (int)(time - last_time));
        last_time = time;
        last_clock = clock_time();
#endif /* ((YLOG_LEVEL) & YLOG_LEVEL_DEBUG) == YLOG_LEVEL_DEBUG */


        if(rtt <= sr_min_rtt) {
          sr_min_rtt = rtt;

          sr_old_diff = sr_time_diff;

          /* symmetric RTT ? - add half of RTT to clock to be closer*/
          set_sr_time(time + rtt / 2);

          if(print_flags & (PRINT_TIME_ONCE | PRINT_TIME_ALWAYS)) {
            static clock_time_t last_print_nbr;
            static clock_time_t last_print_sr;

            print_flags &= ~PRINT_TIME_ONCE;

            YLOG_PRINT("radio time: %"PRIu32" (adjust %"PRId32")  RTT: %d ms\n",
                   (uint32_t)time, (int32_t)(sr_time_diff - sr_old_diff),
                   rtt);
            YLOG_PRINT("time since last print:  router: %"PRId32
                   " ms, radio: %"PRId32" ms\n",
                   (int32_t)(clock_time() - last_print_nbr),
                   (int32_t)(time - last_print_sr));
            last_print_nbr = clock_time();
            last_print_sr = time;
          }

        } else {
          /* increase to adapt min to a bit higher - if needed? */
          sr_min_rtt++;
          if(print_flags & (PRINT_TIME_ONCE | PRINT_TIME_ALWAYS)) {
            print_flags &= ~PRINT_TIME_ONCE;
            YLOG_PRINT("too long RTT for time reading (RTT %d ms)\n", rtt);
          }
        }

      }
      return 1;
    case 'T':
      if(command_context == CMD_CONTEXT_RADIO) {
        YLOG_INFO("Radio tx power is %d dBm\n", (int8_t)data[2]);
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[5];
        int power;
        dectoi(&data[2], len - 2, &power);
        buf[0] = '!';
        buf[1] = 'T';
        buf[2] = power & 0xff;
        write_to_slip(buf, 3);
        return 1;
      }
      return 1;
    case 'P':
      if(command_context == CMD_CONTEXT_RADIO) {
        uint16_t pan_id = ((uint16_t)data[2] << 8) | data[3];
        YLOG_INFO("Pan id is: %u (0x%04x)\n", pan_id, pan_id);
	router_status |= BORDER_ROUTER_STATUS_CONFIGURATION_PAN_ID;
        handler_802154_join(pan_id);

        border_router_radio_set_value(RADIO_PARAM_PAN_ID, pan_id);
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        int pan_id;
        dectoi(&data[2], len - 2, &pan_id);
        /* printf("PAN ID: %u (%04x)\n", pan_id, pan_id); */
        uint8_t buf[5];
        buf[0] = '!';
        buf[1] = 'P';
        buf[2] = (pan_id >> 8) & 0xff;
        buf[3] = pan_id & 0xff;
        write_to_slip(buf, 4);
        handler_802154_join(pan_id);

        border_router_radio_set_value(RADIO_PARAM_PAN_ID, pan_id);
        return 1;
      }
      return 1;
    case 'R':
      if(command_context == CMD_CONTEXT_RADIO) {
        /* We need to know that this is from the slip-radio here. */
        if(br_config_verbose_output > 1) {
          PRINTF("Packet data report for sid:%d st:%d tx:%d\n",
                 data[2], data[3], data[4]);
        }
        packet_sent(data[2], data[3], data[4]);
      }
      return 1;
    case 'E':
      if(command_context == CMD_CONTEXT_RADIO) {
        /* Error from the radio */
        YLOG_ERROR("SR: Unknown command: %c%c (0x%02x%02x)\n",
                   data[2], data[3],
                   data[2], data[3]);
      }
      return 1;
    case 'B':
      if(command_context == CMD_CONTEXT_RADIO) {
        handler_802154_set_beacon_payload((uint8_t *)&data[3], data[2]);
	router_status |= BORDER_ROUTER_STATUS_CONFIGURATION_BEACON;

        /* Beacon has been set - set radio mode to normal mode */
        /* TODO radio mode should be set by UC! */
        border_router_set_radio_mode(RADIO_MODE_NORMAL);
        return 1;
      }
      return 1;
    case 'h':
      /* Print the sniffed packet as hex to standard out for now. */
      if(command_context == CMD_CONTEXT_RADIO) {
        int i;
        printf("h:");
        for(i = 2; i < len; i++) {
          printhex(data[i]);
        }
        printf("\n");
      }
      return 1;
    case 'e':
      /* Energy */
      if(command_context == CMD_CONTEXT_RADIO) {
        uint32_t count = (data[3] << 8) + data[4];
        uint8_t channel = data[2];
        int8_t avg = data[5];
        int8_t max = data[6];
        int8_t min = data[7];
        if(br_config_verbose_output > 1) {
          printf("E: %02d: %02d max %02d min %02d (%u measures)\n",
                 channel, avg, max, min, (unsigned)count);
        }

        if(radio_control_version < 2) {
          /*
           * Disable energy scan mode on older serial radios because
           * when leaving energy scan mode, they might restore a
           * earlier radio channel.
           */
          border_router_set_radio_mode(RADIO_MODE_NORMAL);
          YLOG_ERROR("*** old serial radio: disabling energy scan\n");
        }
      }
      return 1;
    case 'r':
      /* RSSI */
      if(command_context == CMD_CONTEXT_RADIO) {
#if BORDER_ROUTER_PRINT_RSSI
        int pos;
        int8_t rssi;
        if(data[2] == 'A') {
          /* Scan over all channels */
          printf("RSSI:");
          for(pos = 3; pos < len; pos++) {
            rssi = (int8_t)data[pos];
            printf(" %d", rssi);
          }
          printf("\n");

        } else if(data[2] == 'C') {
          /* Scan over single channel */
          printf("RSSI (C):");
          for(pos = 3; pos < len; pos++) {
            rssi = (int8_t)data[pos];
            printf(" %d", rssi);
          }
          printf("\n");
        } else {
          YLOG_ERROR("*** unknown RSSI from radio: %u\n", data[2]);
        }
#endif /* BORDER_ROUTER_PRINT_RSSI */
      }
      return 1;
    case 'm':
      if(command_context == CMD_CONTEXT_RADIO) {
        YLOG_INFO("Radio mode: %u\n", data[2]);
	// Clear router_status variable when radio mode has been set to 0.
	if(data[2] == 0) {
	  router_status = 0;
	} else {
	  router_status |= BORDER_ROUTER_STATUS_CONFIGURATION_MODE;
	}
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        int mode;
        dectoi(&data[2], len - 2, &mode);
        border_router_set_radio_mode(mode & 0xff);
        return 1;
      }
      return 1;
    case 'c':
      if(command_context == CMD_CONTEXT_RADIO) {
        YLOG_INFO("Radio scan mode: %u\n", data[2]);
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[3];
        int mode;
        dectoi(&data[2], len - 2, &mode);
        buf[0] = '!';
        buf[1] = 'c';
        buf[2] = mode & 0xff;
        write_to_slip(buf, 3);
        return 1;
      }
      return 1;
    case 'f':
      if(command_context == CMD_CONTEXT_RADIO) {
        YLOG_INFO("Radio ext mode: %c%u\n", data[2], data[3]);
        return 1;
      }
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[5];
        int value;
        dectoi(&data[3], len - 3, &value);
        buf[0] = '!';
        buf[1] = 'f';
        buf[2] = data[2];
        buf[3] = value & 0xff;
        write_to_slip(buf, 4);
        return 1;
      }
      return 1;
    case 'H':
      if(command_context == CMD_CONTEXT_STDIO) {
        int mode;
        dectoi(&data[2], len - 2, &mode);
        border_router_set_radio_mode(mode == 0 ? 0 : 3);
        return 1;
      }
      return 1;
    case 'd':
      if(command_context == CMD_CONTEXT_STDIO) {
        int verbose;
        uint8_t buf[3];
        dectoi(&data[2], len - 2, &verbose);
        br_config_verbose_output = verbose;
        buf[0] = '!';
        buf[1] = 'd';
        buf[2] = verbose & 0xff;
        write_to_slip(buf, 3);
        return 1;
      }
      if(command_context == CMD_CONTEXT_RADIO) {
        YLOG_INFO("Radio verbose: %u  NBR verbose: %u\n", data[2],
                  br_config_verbose_output);
        return 1;
      }
      return 1;
    case 'v':
      if(command_context == CMD_CONTEXT_RADIO) {
        uint32_t v;
        v  = data[2] << 24;
        v |= data[3] << 16;
        v |= data[4] <<  8;
        v |= data[5];
        radio_control_version = v;
        YLOG_INFO("Radio protocol version: %u\n", v);
      }
      return 1;
    case 'V':
      /* Radio parameter */
      if(command_context == CMD_CONTEXT_RADIO) {
        int type = ((uint16_t)data[2] << 8) | data[3];
        int param = packetutils_to_radio_param(type);
        int value = ((uint16_t)data[4] << 8) | data[5];
        if(param < 0) {
          printf("unknown radio parameter: %d\n", type);
        } else if(param == RADIO_PARAM_RSSI) {
          printf("RSSI: %d\n", (int8_t)(value & 0xff));
        }
      }
      return 1;
    case 'x': {
      if(command_context == CMD_CONTEXT_STDIO) {
	uint8_t buf[10];
	int rate, size, count, pos;
        buf[0] = '!';
        buf[1] = 'x';
        buf[2] = data[2];
	switch(data[2]) {
        case '0':
          /* Reset radio. Forward to serial radio */
          YLOG_INFO("Requesting serial radio to reset the radio!\n");
          write_to_slip(buf, 3);
          return 1;
        case 'c':
          dectoi(&data[3], len - 3, &rate);
          YLOG_INFO("Configuring serial CTS/RTS to %u\n", rate);
          border_router_set_ctsrts(rate);
          return 1;
	case 'u':
	case 'U':
	  dectoi(&data[3], len - 3, &rate);
          if(rate < 1200) {
            YLOG_ERROR("Illegal baudrate for serial radio: %u!\n", rate);
            return 1;
          }
	  YLOG_INFO("Configuring serial radio for %u baud\n", rate);
          border_router_set_baudrate(rate, data[2] == 'U' ? 1 : 0);
	  return 1;
	case 'r':
	  /* Forward to serial radio */
	  if(len > 2) {
	    dectoi(&data[3], len - 3, &pos);
	    if(pos > 0 && pos < 3) {
	      YLOG_INFO("Requesting serial radio to reboot to image %u!\n", pos);
	      printf("requesting serial radio to reboot to image %u!\n", pos);
	      buf[3] = pos & 0xff;
	      write_to_slip(buf, 4);
	      return 1;
	    }
	  }
	  YLOG_INFO("Requesting serial radio to reboot!\n");
	  write_to_slip(data, len);
	  return 1;
        case 'F':
          /* Set system fan enabled */
          dectoi(&data[3], len - 3, &rate);
          YLOG_DEBUG("Setting system fan speed to %u\n", rate);
          buf[3] = (rate >> 8) & 0xff;
          buf[4] = rate & 0xff;
          write_to_slip(buf, 5);
          return 1;
        case 'W':
          /* Set radio watchdog */
          dectoi(&data[3], len - 3, &rate);
          YLOG_DEBUG("Setting radio watchdog to %u\n", rate);
          border_router_set_radio_watchdog(rate);
          return 1;
        case 'e':
          dectoi(&data[3], len - 3, &rate);
          YLOG_DEBUG("Setting frontpanel error code: %u\n", rate);
          buf[3] = (rate >> 8) & 0xff;
          buf[4] = rate & 0xff;
          write_to_slip(buf, 5);
          return 1;
        case 'l':
          dectoi(&data[3], len - 3, &rate);
          YLOG_DEBUG("Setting frontpanel info: %u\n", rate);
          buf[3] = (rate >> 8) & 0xff;
          buf[4] = rate & 0xff;
          write_to_slip(buf, 5);
          return 1;
	case 'd':
	  dectoi(&data[3], len - 3, &rate);
	  YLOG_DEBUG("Configuring serial radio for fragment delay %u\n", rate);
	  buf[3] = (rate >> 8) & 0xff;
	  buf[4] = rate & 0xff;
	  write_to_slip(buf, 5);
	  return 1;
	case 'b':
	  dectoi(&data[3], len - 3, &rate);
	  YLOG_DEBUG("Configuring serial radio for fragment size %u\n", rate);
	  buf[3] = (rate >> 8) & 0xff;
	  buf[4] = rate & 0xff;
	  write_to_slip(buf, 5);
	  return 1;
	case 's':
	  rate = 62;
	  size = 64;
	  dectoi(&data[3], len - 3, &count);
	  pos = index_of(',', data, 3, len);
	  if(pos > 0) {
	    dectoi(&data[pos + 1], len - (pos + 1), &size);
	    pos = index_of(',', data, pos + 1, len);
	    if(pos > 0) {
	      dectoi(&data[pos + 1], len - (pos + 1), &rate);
	    }
	  }
	  buf[3] = (count >> 8) & 0xff;
	  buf[4] = count & 0xff;
	  buf[5] = (size >> 8) & 0xff;
	  buf[6] = size & 0xff;
	  buf[7] = (rate >> 8) & 0xff;
	  buf[8] = rate & 0xff;
	  YLOG_DEBUG("Requesting serial radio to send %u data of size %u every %u msec\n",
		 count, size, rate);
	  write_to_slip(buf, 9);
	  return 1;
	case 'x':
          /* Forward command directly to serial radio */
          YLOG_DEBUG("forwards %u bytes to serial radio\n", len);
          write_to_slip(data, len);
          return 1;
	}
      }
      if(command_context == CMD_CONTEXT_RADIO) {
	static uint16_t last_seqno;
	switch(data[2]) {
	case 'S': {
	  /* Data from the radio */
          uint16_t seqno;
          seqno = (data[3] << 8) + data[4];
	  if(seqno != last_seqno + 1) {
	    YLOG_DEBUG("RECV %u bytes, %u (lost %u)\n", len, seqno,
		   seqno - (last_seqno + 1));
	  } else {
	    /* printf("RECV %u bytes, %u\n", len, seqno); */
	    putchar('.');
	  }
	  last_seqno = seqno;
	  break;
        }
        case 'W': {
          uint16_t sec;
          /* The radio is telling us the radio watchdog soon will expire */
          sec = (data[3] << 8) + data[4];
          YLOG_INFO("THE RADIO WATCHDOG WILL REBOOT SYSTEM IN %u SECONDS!\n", sec);

          /*
           * Only call the platform reboot script if this is not a
           * remote border router.
           */
          if(! enc_dev_is_remote()) {
            int reboot_status;

            YLOG_INFO(">>> executing %s\n", BORDER_ROUTER_REBOOT_PLATFORM_COMMAND);
            reboot_status = system(BORDER_ROUTER_REBOOT_PLATFORM_COMMAND);
            YLOG_INFO(">>> executing %s returned %d\n", BORDER_ROUTER_REBOOT_PLATFORM_COMMAND, reboot_status);
          }
          break;
        }
	}
      }
      return 1;
    }
    }
  } else if(data[0] == '?') {
    PRINTF("Got request message of type %c\n", data[1]);
    switch(data[1]) {
    case 'M':
      /**
       * Requesting MAC address from radio is an internal command and
       * should not be called by user.
       *
       * Receiving MAC address from radio might trig a restart of the
       * network stack and is part of the startup of for example
       * native debug node.
       */
      /* if(command_context == CMD_CONTEXT_STDIO) { */
      /*   uint8_t buf[4]; */
      /*   buf[0] = '?'; */
      /*   buf[1] = 'M'; */
      /*   write_to_slip(buf, 2); */
      /* } */
      /* return 1; */
      return 0;
    case 'C':
    case 'T':
    case 'P':
    case 'm':
    case 'c':
    case 'f':
    case 'i':
    case 'x':
      if(command_context == CMD_CONTEXT_STDIO) {
        /* send on! */
        write_to_slip(data, len);
        return 1;
      }
      return 1;
    case 't':
      if(command_context == CMD_CONTEXT_STDIO) {
        print_flags |= PRINT_TIME_ONCE;
        border_router_request_radio_time();
      }
      return 1;
    case 'v':
      if(command_context == CMD_CONTEXT_STDIO) {
        /* send on! */
        write_to_slip(data, len);
        YLOG_INFO("Border router version: %u\n", BORDER_ROUTER_CONTROL_API_VERSION);
        return 1;
      }
      return 1;
    case 'V':
      if(command_context == CMD_CONTEXT_STDIO) {
        /* TODO Radio parameter */
        return 1;
      }
      return 1;
    case 'd':
      if(command_context == CMD_CONTEXT_STDIO) {
        uint8_t buf[3];
        buf[0] = '?';
        buf[1] = 'd';
        write_to_slip(buf, 2);
        return 1;
      }
      return 1;
    case 'S': {
      if(command_context == CMD_CONTEXT_STDIO) {
	uint8_t buf[3];
	border_router_print_stat();
	buf[0] = '?';
	buf[1] = 'S';
	write_to_slip(buf, 2);
	return 1;
      }
      return 1;
    }
    }

  } else if(command_context == CMD_CONTEXT_STDIO) {

    if(strcmp("rpl", (const char *)data) == 0) {
      rpl_dag_t *dag = rpl_get_any_dag();
      rpl_instance_t *instance;
      if(dag != NULL) {
        instance = dag->instance;
        printf("DAG: rank:%d version:%d\n", dag->rank, dag->version);
        printf("    dio_sent: %d dio_recv: %d dio_totint: %d\n",
               instance->dio_totsend, instance->dio_totrecv,
               instance->dio_totint);
        /* printf("    dao_sent: %d dao_recv: %d dao_acc: %d dao_nack_sent: %d dao_nopath: %d\n", */
        /*        instance->dao_totsend, instance->dao_totrecv, */
        /*        instance->dao_totaccepted, instance->dao_nack_totsend, */
        /*        instance->dao_nopath_totsend); */
      } else {
        printf("No DAG found - not joined yet\n");
      }

      rpl_print_neighbor_list();
      return 1;

    } else if(strcmp("routes", (const char *)data) == 0) {
      uip_ds6_route_t *r;
      uip_ds6_defrt_t *defrt;
      uip_ipaddr_t *ipaddr;
      defrt = NULL;
      if((ipaddr = uip_ds6_defrt_choose()) != NULL) {
        defrt = uip_ds6_defrt_lookup(ipaddr);
      }
      if(defrt != NULL) {
        printf("DefRT: :: -> ");
        uip_debug_ipaddr_print(&defrt->ipaddr);
        if(defrt->isinfinite) {
          printf(" (infinite lifetime)\n");
        } else {
          printf(" lifetime: %lu sec\n", stimer_remaining(&defrt->lifetime));
        }
      } else {
        printf("DefRT: :: -> NULL\n");
      }

      printf("Routes:\n");
      for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
        printf(" ");
        uip_debug_ipaddr_print(&r->ipaddr);
        printf(" -> ");
        if(uip_ds6_route_nexthop(r) != NULL) {
          uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
        } else {
          printf("NULL");
        }
        if(r->state.lifetime < 600) {
          printf(" %d s\n", r->state.lifetime);
        } else {
          printf(" >600 s\n");
        }
      }
      printf("\n");
      return 1;

    } else if(strcmp("nbr", (const char *)data) == 0) {
      uip_ds6_nbr_t *nbr;
      rpl_instance_t *def_instance;
      const uip_lladdr_t *lladdr;
      rpl_parent_t *p;
      uint16_t rank;

      def_instance = rpl_get_default_instance();

      printf("Neighbors IPv6         \t  sec  state       rank      RPL fg\n");
      for(nbr = nbr_table_head(ds6_neighbors);
          nbr != NULL;
          nbr = nbr_table_next(ds6_neighbors, nbr)) {
        uip_debug_ipaddr_print(&nbr->ipaddr);
        lladdr = uip_ds6_nbr_get_ll(nbr);
        p = nbr_table_get_from_lladdr(rpl_parents, (const linkaddr_t*)lladdr);
        rank = p != NULL ? p->rank : 0;
        if(stimer_expired(&nbr->reachable)) {
          printf("\t %5c ", '-');
        } else {
          printf("\t %5lu ", stimer_remaining(&nbr->reachable));
        }
        printf("%-10s ", get_nbr_state_name(nbr->state));
        printf(" %d.%02d[%3d] %c",
               rank / 128, (int)(rank * 100L / 128) % 100, rank,
               p != NULL ? 'P' : ' ');
        if(def_instance != NULL && def_instance->current_dag != NULL &&
           def_instance->current_dag->preferred_parent == p) {
          printf("*");
        } else {
          printf(" ");
        }
        if(p != NULL) {
          printf(" %2x", p->flags);
        } else {
          printf("   ");
        }
        printf("\n");
      }
      printf("\n");
      return 1;

    } else if(strncmp("suppressdio", (char *)data, 11) == 0) {
      int value;
      net_control_key_t control_key;
      value = -1;
      for(data += 11, len -= 11; *data == ' ' && len > 0; data++, len--);
      if(len > 0) {
        dectoi(data, len, &value);
      }
      control_key = get_net_control_key();
      if(value < 0 || control_key == NET_CONTROL_KEY_NONE) {
        if(net_control_get(NET_CONTROL_RPL_SUPPRESS_DIO)) {
          printf("DIO is suppressed\n");
        } else {
          printf("DIO is not suppressed\n");
        }
      } else if(value > 0) {
        printf("Suppressing DIO\n");
        net_control_set(NET_CONTROL_RPL_SUPPRESS_DIO, control_key, 1);
      } else {
        printf("No longer suppressing DIO\n");
        net_control_set(NET_CONTROL_RPL_SUPPRESS_DIO, control_key, 0);
        if(net_control_get(NET_CONTROL_RPL_SUPPRESS_DIO)) {
          printf("DIO is still suppressed by another module\n");
        }
      }
      return 1;
    } else if(strncmp("suppressbeacons", (char *)data, 15) == 0) {
      int value;
      net_control_key_t control_key;
      value = -1;
      for(data += 15, len -= 15; *data == ' ' && len > 0; data++, len--);
      if(len > 0) {
        dectoi(data, len, &value);
      }
      control_key = get_net_control_key();
      if(value < 0 || control_key == NET_CONTROL_KEY_NONE) {
        if(net_control_get(NET_CONTROL_SUPPRESS_BEACON)) {
          printf("Beacon replies are suppressed\n");
        } else {
          printf("Beacon replies are not suppressed\n");
        }
      } else if(value > 0) {
        printf("Suppressing beacon replies\n");
        net_control_set(NET_CONTROL_SUPPRESS_BEACON, control_key, 1);
      } else {
        printf("No longer suppressing beacon replies\n");
        net_control_set(NET_CONTROL_SUPPRESS_BEACON, control_key, 0);
        if(net_control_get(NET_CONTROL_SUPPRESS_BEACON)) {
          printf("Beacon replies are still suppressed by another module\n");
        }
      }
      return 1;

    } else if(strncmp("latency", (char *)data, 7) == 0) {
      int value;

      value = -1;
      for(data += 7, len -= 7; *data == ' ' && len > 0; data++, len--);
      if(len > 0) {
        dectoi(data, len, &value);
      }
      if(value >= 0) {
        printf("Latency stats output is now set to %u\n", value);
        latency_stats_set_output_level(value);
      } else {
        printf("Latency stats output is currently set to %u\n",
               latency_stats_get_output_level());
      }
      return 1;

    } else if(strncmp("log", (char *)data, 3) == 0 &&
              (len == 3 || data[3] == ' ')) {
      uint8_t log;
      log = border_router_rdc_get_logging();
      for(data += 3, len -= 3; *data == ' ' && len > 0; data++, len--);
      if(len > 0) {
        if(strcmp("all", (char *)data) == 0) {
          log ^= BORDER_ROUTER_RDC_LOG_TX | BORDER_ROUTER_RDC_LOG_RX;
        } else if(strcmp("tx", (char *)data) == 0) {
          log ^= BORDER_ROUTER_RDC_LOG_TX;
        } else if(strcmp("rx", (char *)data) == 0) {
          log ^= BORDER_ROUTER_RDC_LOG_RX;
        } else if(strcmp("time", (char *)data) == 0) {
          print_flags ^= PRINT_TIME_ALWAYS;
        } else {
          printf("unknown log type: %s\n", (char *)data);
          return 1;
        }
        border_router_rdc_set_logging(log);
      }
      printf("RDC logging: 0x%x  print flags: 0x%x\n", log, print_flags);
      return 1;
    } else if(strcmp("rssi", (char *)data) == 0) {
      uint8_t buf[4];
      int p;

      p = packetutils_from_radio_param(RADIO_PARAM_RSSI);
      if(p >= 0) {
        buf[0] = '?';
        buf[1] = 'V';
        buf[2] = (p >> 8) & 0xff;
        buf[3] = p & 0xff;
        write_to_slip(buf, 4);
      }
      return 1;

    } else if(strcmp("help", (char *)data) == 0) {
      printf("No command list available\n");
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
border_router_cmd_output(const uint8_t *data, int data_len)
{
  int i;
  YLOG_INFO("CMD output: ");
  for(i = 0; i < data_len; i++) {
    printf("%c", data[i]);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
void
border_router_cmd_error(cmd_error_t error, const uint8_t *data, int data_len)
{
  if(error == CMD_ERROR_UNKNOWN_COMMAND) {
    YLOG_ERROR("Unknown command: %c%c (0x%02x%02x%02x%02x) (%d bytes)\n",
               data[0], data[1], data[0], data[1], data[2], data[3],
               data_len);
  } else {
    YLOG_ERROR("command error %d: %c%c (0x%02x%02x)\n",
               error, data[0], data[1], data[0], data[1]);
  }
}
/*---------------------------------------------------------------------------*/
uint32_t
border_router_get_status()
{
  return router_status;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_cmd_process, ev, data)
{
  PROCESS_BEGIN();
  PRINTF("Started br-cmd process\n");
  while(1) {
    PROCESS_YIELD();
    if(ev == serial_line_event_message && data != NULL) {
      command_context = CMD_CONTEXT_STDIO;
      cmd_input(data, strlen((char *)data));
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
