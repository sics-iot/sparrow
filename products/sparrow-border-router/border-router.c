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
 *
 */

/**
 * \file
 *         border-router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/watchdog.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"

#include "net/netstack.h"
#include "net/mac/frame802154.h"
#include "net/mac/handler-802154.h"
#include "dev/slip.h"
#include "cmd.h"
#include "border-router.h"
#include "border-router-cmds.h"
#include "brm-stats.h"
#include "br-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "udp-cmd.h"
#include "sparrow-oam.h"

void enc_dev_init(void);

#define YLOG_LEVEL YLOG_LEVEL_INFO
#define YLOG_NAME  "nbr"
#include "ylog.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

extern uint32_t radio_control_version;

extern unsigned long slip_sent;
extern unsigned long slip_sent_to_fd;
extern unsigned int  slip_max_buffer_usage;
long slip_buffered(void);

static int64_t radio_diff;

static uip_ipaddr_t prefix;
static uip_ipaddr_t dag_id;

static uint8_t prefix_set;

static uint8_t dag_init_version = 0;
static uint8_t has_dag_version = 0;

extern int contiki_argc;
extern char **contiki_argv;

uint8_t radio_info = 0;

static uint8_t radio_mode = RADIO_MODE_IDLE;

CMD_HANDLERS(border_router_cmd_handler);

PROCESS(border_router_process, "Border router process");

AUTOSTART_PROCESSES(&border_router_process, &border_router_cmd_process);

#if WEBSERVER == 1
/* Use simple webserver with only one page */
#include "httpd-simple.h"
PROCESS(webserver_nogui_process, "Web server");
PROCESS_THREAD(webserver_nogui_process, ev, data)
{
  PROCESS_BEGIN();

  httpd_init();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    httpd_appcall(data);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static const char *TOP = "<html><head><title>ContikiRPL</title></head><body>\n";
static const char *BOTTOM = "</body></html>\n";
static char buf[8192];
static int blen, bret;
#define ADD(...) do {                                                   \
    bret = snprintf(&buf[blen], sizeof(buf) - blen, __VA_ARGS__);       \
    if(bret > 0 && bret < sizeof(buf) - blen) {                         \
      blen += bret;                                                     \
    }                                                                   \
  } while(0)

#define SEND_BUF(sout) do {                                             \
    if(prepare_buf()) {                                                 \
      SEND_STRING(sout, buf);                                           \
    }                                                                   \
  } while(0)
/*---------------------------------------------------------------------------*/
static int
prepare_buf(void)
{
  static uint8_t has_warned = 0;
  if(blen == 0) {
    return 0;
  }

  if(blen >= sizeof(buf)) {
    blen = sizeof(buf) - 1;
    buf[blen - 3] = '.';
    buf[blen - 2] = '.';
    buf[blen - 1] = '.';

    if(has_warned < 50) {
      has_warned++;
      YLOG_ERROR("Too large web page requested - truncating\n");
    }
  }
  /* Make sure the string is terminated */
  buf[blen] = '\0';

  /* Clear position to prepare for next additions */
  blen = 0;

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
ipaddr_add(const uip_ipaddr_t *addr)
{
  uint16_t a;
  int i, f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0 && sizeof(buf) - blen > 2) {
        buf[blen++] = ':';
        buf[blen++] = ':';
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0 && blen < sizeof(buf) - 1) {
        buf[blen++] = ':';
      }
      ADD("%x", a);
    }
  }
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(generate_routes(struct httpd_state *s))
{
  static int full;
  uip_ds6_route_t *r;
  uip_ds6_nbr_t *nbr;

  PSOCK_BEGIN(&s->sout);

  /* if filename start with /r, just print routes */
  if(s->filename[1] == 'r') {
    full = FALSE;
  } else {
    full = TRUE;
  }

  blen = 0;
  if(full) {
    SEND_STRING(&s->sout, TOP);
    ADD("Neighbors<pre>\n");
    for(nbr = nbr_table_head(ds6_neighbors);
        nbr != NULL;
        nbr = nbr_table_next(ds6_neighbors, nbr)) {
      ipaddr_add(&nbr->ipaddr);
      ADD("\n");
    }
    SEND_BUF(&s->sout);
    ADD("</pre>\nRoutes<pre>\n");
  }

  for(r = uip_ds6_route_head();
      r != NULL;
      r = uip_ds6_route_next(r)) {
    ipaddr_add(&r->ipaddr);
    if(full) {
      ADD("/%u (via ", r->length);
      ipaddr_add(uip_ds6_route_nexthop(r));
      if(r->state.lifetime < 6000) {
        ADD(") %lu:%02lus\n", (unsigned long)r->state.lifetime / 60,
            (unsigned long)r->state.lifetime % 60);
      } else {
        ADD(")\n");
      }
    } else {
      ADD("\n");
    }
  }
  SEND_BUF(&s->sout);

  if(full) {
    ADD("</pre>\n");
    SEND_BUF(&s->sout);

    SEND_STRING(&s->sout, BOTTOM);
  }
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
httpd_simple_script_t
httpd_simple_get_script(const char *name)
{
  return generate_routes;
}
#endif /* WEBSERVER == 1 */
/*---------------------------------------------------------------------------*/
static void
send_periodic(void)
{
  udp_cmd_send_poll_response();
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTA("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINTA(" %p: => ", &uip_ds6_if.addr_list[i]);
      uip_debug_ipaddr_print(&(uip_ds6_if.addr_list[i]).ipaddr);
      PRINTA("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
int
border_router_is_slave(void)
{
  return br_config_is_slave;
}
/*---------------------------------------------------------------------------*/
/* The time is in milliseconds and is 64-bits                                */
/*---------------------------------------------------------------------------*/
uint64_t
border_router_get_radio_time(void)
{
  return clock_time() - radio_diff;
}
/*---------------------------------------------------------------------------*/
void
border_router_set_radio_diff(int64_t r)
{
  radio_diff = r;
}
/*---------------------------------------------------------------------------*/
void
border_router_set_slave(int slave)
{
  br_config_is_slave = slave != 0;
}
/*---------------------------------------------------------------------------*/
int
border_router_is_slave_active(void)
{
#if HAVE_BORDER_ROUTER_SERVER
  return border_router_server_has_client_connection();
#else /* HAVE_BORDER_ROUTER_SERVER */
  return 0;
#endif /* HAVE_BORDER_ROUTER_SERVER */
}
/*---------------------------------------------------------------------------*/
void
border_router_set_mac(const uint8_t *data)
{
  /* Setting MAC address is handled when processing radio info */
  printf("Radio MAC address is ");
  uip_debug_lladdr_print((const uip_lladdr_t *)&data[2]);
  printf("\n");
}
/*---------------------------------------------------------------------------*/
void
border_router_request_radio_time(void)
{
  uint64_t now;
  uint8_t buf[10];
  int i;
  buf[0] = '?';
  buf[1] = 't';
  now = clock_time();
  for(i = 0; i < 8; i++) {
    buf[9 - i] = now & 0xff;
    now = now >> 8;
  }
  write_to_slip(buf, 10);
}
/*---------------------------------------------------------------------------*/
uint8_t
border_router_get_radio_mode(void)
{
  return radio_mode;
}
/*---------------------------------------------------------------------------*/
void
border_router_set_radio_mode(uint8_t mode)
{
  uint8_t buf[3];
  buf[0] = '!';
  buf[1] = 'm';
  buf[2] = mode;
  radio_mode = mode;
  write_to_slip(buf, 3);
}
/*---------------------------------------------------------------------------*/
void
border_router_reset_radio(void)
{
  uint8_t buf[3];
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = '0';
  write_to_slip(buf, 3);

  if(radio_mode == RADIO_MODE_NORMAL) {
    /* Restore normal radio mode */
    border_router_set_radio_mode(RADIO_MODE_NORMAL);
  }
}
/*---------------------------------------------------------------------------*/
void
border_router_set_radio_watchdog(uint16_t seconds)
{
  uint8_t buf[5];
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = 'W';
  buf[3] = (seconds >> 8) & 0xff;
  buf[4] = seconds & 0xff;
  write_to_slip(buf, 5);
}
/*---------------------------------------------------------------------------*/
void
border_router_set_frontpanel_info(uint16_t info)
{
  uint8_t buf[5];
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = 'l';
  buf[3] = (info >> 8) & 0xff;
  buf[4] = info & 0xff;
  write_to_slip(buf, 5);
}
/*---------------------------------------------------------------------------*/
static uint8_t pending_on;
static uint8_t pending_off;
static unsigned int original_baudrate;
static unsigned int pending_baudrate;
static struct ctimer pending_on_timer;
static struct ctimer pending_off_timer;
#define PENDING_CTSRTS       (1 << 0)
#define PENDING_BAUDRATE     (1 << 1)
#define PENDING_START_DELAY  (CLOCK_SECOND / 2)
#define PENDING_TEST_PERIOD  (CLOCK_SECOND * 30)
/*---------------------------------------------------------------------------*/
static void
pending_on_callback(void *ptr)
{
  if(pending_on & PENDING_CTSRTS) {
    pending_on &= ~PENDING_CTSRTS;
    serial_set_ctsrts(1);
  }

  if(pending_on & PENDING_BAUDRATE) {
    pending_on &= ~PENDING_BAUDRATE;
    serial_set_baudrate(pending_baudrate);
  }
}
/*---------------------------------------------------------------------------*/
static void
pending_off_callback(void *ptr)
{
  if(pending_off & PENDING_CTSRTS) {
    pending_off &= ~PENDING_CTSRTS;
    serial_set_ctsrts(0);
    YLOG_INFO("Disabling CTS/RTS\n");
  }

  if(pending_off & PENDING_BAUDRATE) {
    pending_off &= ~PENDING_BAUDRATE;
    serial_set_baudrate(original_baudrate);
    YLOG_INFO("Restoring baudrate\n");
  }
}
/*---------------------------------------------------------------------------*/
void
border_router_set_ctsrts(uint32_t ctsrts)
{
  uint8_t buf[8];
  pending_on &= ~PENDING_CTSRTS;
  pending_off &= ~PENDING_CTSRTS;
  if(ctsrts == 2) {
    printf("Configuring serial radio to use CTS/RTS\n");
    pending_on |= PENDING_CTSRTS;
    ctimer_set(&pending_on_timer, PENDING_START_DELAY,
               pending_on_callback, NULL);
  } else if(ctsrts == 1) {
    printf("Configuring serial radio to test CTS/RTS\n");
    pending_on |= PENDING_CTSRTS;
    ctimer_set(&pending_on_timer, PENDING_START_DELAY,
               pending_on_callback, NULL);
    pending_off |= PENDING_CTSRTS;
    ctimer_set(&pending_off_timer, PENDING_TEST_PERIOD,
               pending_off_callback, NULL);
  } else {
    printf("Configuring serial radio to not use CTS/RTS\n");
    ctsrts = 0;
    serial_set_ctsrts(0);
  }
  /* Configure serial radio */
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = 'c';
  buf[3] = ctsrts & 0xff;
  write_to_slip(buf, 4);
}
/*---------------------------------------------------------------------------*/
void
border_router_set_baudrate(unsigned rate, int testonly)
{
  uint8_t buf[8];
  if(rate < 1200) {
    YLOG_ERROR("Illegal baudrate: %u!\n", rate);
    return;
  }
  buf[0] = '!';
  buf[1] = 'x';
  buf[2] = testonly ? 'U' : 'u';
  buf[3] = (rate >> 24) & 0xff;
  buf[4] = (rate >> 16) & 0xff;
  buf[5] = (rate >> 8) & 0xff;
  buf[6] = rate & 0xff;
  write_to_slip(buf, 7);
  if(original_baudrate == 0) {
    original_baudrate = serial_get_baudrate();
  }
  pending_baudrate = rate;
  if(testonly) {
    pending_off |= PENDING_BAUDRATE;
    ctimer_set(&pending_off_timer, PENDING_TEST_PERIOD,
               pending_off_callback, NULL);
  } else {
    pending_off &= ~PENDING_BAUDRATE;
  }
  pending_on |= PENDING_BAUDRATE;
  ctimer_set(&pending_on_timer, PENDING_START_DELAY,
             pending_on_callback, NULL);
}
/*---------------------------------------------------------------------------*/
void
border_router_print_stat()
{
  print_local_addresses();
  YLOG_INFO("ENCAP: received %u bytes payload, %u errors\n",
            BRM_STATS_GET(BRM_STATS_ENCAP_RECV),
            BRM_STATS_GET(BRM_STATS_ENCAP_ERRORS));
  YLOG_INFO("SLIP: received %u bytes, %u frames, %u overflows, %u dropped\n",
            BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_RECV),
            BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_FRAMES),
            BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_OVERFLOWS),
            BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_DROPPED));
  YLOG_INFO("SLIP: sent %lu bytes, %lu frames, %ld packets pending\n",
            slip_sent_to_fd, slip_sent, slip_buffered());
  YLOG_INFO("SLIP: max read buffer usage: %lu bytes\n", slip_max_buffer_usage);
}
/*---------------------------------------------------------------------------*/
static void
set_prefix_64(const uip_ipaddr_t *prefix_64)
{
  /* configure the dag_id as this nodes global IPv6 address */
  memcpy(&prefix, prefix_64, 16);
  memcpy(&dag_id, prefix_64, 16);

  prefix_set = 1;
  uip_ds6_set_addr_iid(&dag_id, &uip_lladdr);
  uip_ds6_addr_add(&dag_id, 0, ADDR_AUTOCONF);
}
/*---------------------------------------------------------------------------*/
static int
br_join_dag(const rpl_dio_t *dio)
{
  static uint8_t last_version;

  if(uip_ip6addr_cmp(&dio->dag_id, &dag_id)) {
    /* Only print if we see another network */
    if(dag_init_version != dio->version && last_version != dio->version) {
      YLOG_DEBUG("DIO: Version:%d InstanceID:%d\n", dio->version, dio->instance_id);
      YLOG_DEBUG("Found network with me as root!\n");
      last_version = dio->version;
    }
    /* here we should check that the last dag_init_version was less than
     * dio->version so that we get the highest version to start with...
     * main problem is if the lollipop version is in the start region so that
     * it is impossible to detect reboot */
    if(dio->version > RPL_LOLLIPOP_CIRCULAR_REGION) {
      dag_init_version = dio->version;
      has_dag_version = 1;
    }
  } else {
    YLOG_DEBUG("DIO: Version:%d InstanceID:%d\n", dio->version, dio->instance_id);
    YLOG_DEBUG("Found network with other root ");
    PRINT6ADDR(&dio->dag_id);
    PRINTF(" me:");
    PRINT6ADDR(&dag_id);
    PRINTF("\n");
  }

  /* never join */
  return 0;
}
/*---------------------------------------------------------------------------*/
void
border_router_request_radio_version(void)
{
  uint8_t buffer[10];
  buffer[0] = '?';
  buffer[1] = 'v';
  buffer[2] = buffer[3] = buffer[4] = 0;
  buffer[5] = BORDER_ROUTER_CONTROL_API_VERSION;
  /* Request radio protocol version */
  write_to_slip(buffer, 6);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_process, ev, data)
{
  static struct etimer et;
  static uint64_t wait_started;
  static int wait_turns;
  static int dis;
  rpl_dag_t *dag;

  PROCESS_BEGIN();

  watchdog_init();
  /* Start watchdog immediately */
  /* watchdog_start(); */

  prefix_set = 0;
  dis = 2; /* send a couple of DIS at startup to detect any existing network */

  PROCESS_PAUSE();

  watchdog_periodic();

  YLOG_INFO("RPL-Border router started\n");
  br_config_handle_arguments(contiki_argc, contiki_argv);

  YLOG_DEBUG("Registering Join Callback\n");
  rpl_set_join_callback(br_join_dag);

  udp_cmd_init();

  /* First init enc-dev so we can get the mac address from the radio */
  enc_dev_init();

  etimer_set(&et, CLOCK_SECOND / 32);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  watchdog_periodic();

  if(br_config_run_command != NULL) {
    YLOG_DEBUG("Calling command '%s'\n", br_config_run_command);
    command_context = CMD_CONTEXT_STDIO;
    cmd_input((uint8_t *)br_config_run_command, strlen(br_config_run_command));
    etimer_set(&et, 2 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    /* Command executing - exiting border router */
    exit(EXIT_SUCCESS);
  }

#ifdef HAVE_BORDER_ROUTER_CTRL
  border_router_ctrl_init();
#endif /* HAVE_BORDER_ROUTER_CTRL */

  YLOG_INFO("Border router:\n");
  YLOG_INFO("  sw revision: %s\n", sparrow_getswrevision());
  YLOG_INFO("  Compiled at: %s\n", sparrow_getcompiletime());

#if CLOCK_USE_CLOCK_GETTIME
  YLOG_INFO("  System clock: clock_gettime()\n");
#endif
#if CLOCK_USE_RELATIVE_TIME
  YLOG_INFO("  System clock: relative time\n");
#endif
  YLOG_INFO("Network Configuration:\n");
  printf("  %u routes\n  %u neighbors\n  %u default routers\n  %u prefixes\n"
         "  %u unicast addresses\n  %u multicast addresses\n"
         "  %u anycast addresses\n",
         UIP_DS6_ROUTE_NB, NBR_TABLE_MAX_NEIGHBORS, UIP_DS6_DEFRT_NB,
         UIP_DS6_PREFIX_NB, UIP_DS6_ADDR_NB, UIP_DS6_MADDR_NB, UIP_DS6_AADDR_NB);
  printf("  %lu seconds ND6 reachable\n",
         (unsigned long)(UIP_ND6_REACHABLE_TIME / 1000));
#if RPL_DEFAULT_LIFETIME == 0xff && RPL_DEFAULT_LIFETIME_UNIT == 0xffff
  printf("  Infinite default lifetime for RPL\n");
#else
  printf("  %u default lifetime units, %u seconds per unit, for RPL\n",
         RPL_DEFAULT_LIFETIME, RPL_DEFAULT_LIFETIME_UNIT);
#endif
  border_router_request_radio_version();

#ifdef BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING
  /* Set initial front panel info at first start */
  border_router_set_frontpanel_info(BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING);
#endif /* BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING */

  while((radio_info & RADIO_INFO_NEEDED_INFO) != RADIO_INFO_NEEDED_INFO) {
    etimer_set(&et, CLOCK_SECOND);
    if(radio_info != 0) {
      YLOG_DEBUG("Requesting radio info (0x%02x)...\n", RADIO_INFO_NEEDED_INFO - radio_info);
    } else {
      YLOG_DEBUG("Requesting radio info...\n");
    }
    udp_cmd_request_radio_info();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    watchdog_periodic();
  }
  YLOG_DEBUG("Radio info received!\n");
  YLOG_INFO("Radio sw revision: %s\n", instance_brm_get_radio_sw_revision());
  YLOG_INFO("      bootloader:  %lu  capabilities: 0x%lx%08lx\n",
            instance_brm_get_radio_bootloader_version(),
            (unsigned long)((instance_brm_get_radio_capabilities() >> 32ULL) & 0xffffffffUL),
            (unsigned long)(instance_brm_get_radio_capabilities() & 0xffffffffUL));

  if(((instance_brm_get_radio_capabilities() & GLOBAL_CHASSIS_CAPABILITY_CTS_RTS) &&
      ((instance_brm_get_radio_capabilities() & GLOBAL_CHASSIS_CAPABILITY_MASK) == 0))) {
    YLOG_INFO("Enabling CTS/RTS\n");

    border_router_set_ctsrts(1);

    etimer_set(&et, CLOCK_SECOND / 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    YLOG_DEBUG("Waiting for radio info with CTS/RTS\n");

    /* Request number of instances again to test the serial communication */
    radio_info &= ~RADIO_INFO_INSTANCES;
    wait_turns = 0;
    while((radio_info & RADIO_INFO_NEEDED_INFO) != RADIO_INFO_NEEDED_INFO) {
      etimer_set(&et, CLOCK_SECOND);
      udp_cmd_request_radio_info();
      wait_turns++;
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      if(wait_turns > 45) {
        break;
      }
      watchdog_periodic();
    }

    if(wait_turns > 15) {
      YLOG_INFO("No working CTS/RTS after %d seconds\n", wait_turns + 1);
    } else {
      YLOG_INFO("CTS/RTS seems to work after %d seconds!\n", wait_turns + 1);
      border_router_set_ctsrts(2);
      etimer_set(&et, CLOCK_SECOND / 2);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    }
  }

  watchdog_periodic();

  if(!br_config_wait_for_address) {
    /* Stand alone mode - configure radio to defaults */
    border_router_set_radio_mode(RADIO_MODE_NORMAL);
  }

  /* Have all info from radio, init OAM and UDP server */
  udp_cmd_start();
  sparrow_oam_init();

  if(radio_control_version == 0) {
    border_router_request_radio_version();
  }

  /* The border router runs with a 100% duty cycle in order to ensure high
     packet reception rates. */
  NETSTACK_MAC.off(1);

#ifdef HAVE_BORDER_ROUTER_SERVER
  border_router_server_init();
#endif /* HAVE_BORDER_ROUTER_SERVER */

  if(br_config_is_slave) {
    YLOG_INFO("in slave mode\n");

    while(br_config_is_slave || border_router_is_slave_active()) {
      etimer_set(&et, CLOCK_SECOND * 30);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
#if BORDER_ROUTER_SET_RADIO_WATCHDOG > 0 && defined(HAVE_BORDER_ROUTER_SERVER)
      /* Keep the radio watchdog updated if in slave mode without server connection. */
      if(!border_router_is_slave_active()) {
        border_router_set_radio_watchdog(BORDER_ROUTER_SET_RADIO_WATCHDOG);
      }
#endif /* BORDER_ROUTER_SET_RADIO_WATCHDOG > 0 && defined(HAVE_BORDER_ROUTER_SERVER) */
    }
  }

#if WEBSERVER > 0
  process_start(&webserver_nogui_process, NULL);
#endif /* WEBSERVER > 0 */

  if(br_config_wait_for_address) {
    YLOG_DEBUG("RPL-Border router wait for address\n");
    wait_turns = 0;
    wait_started = uptime_read();
    while(br_config_wait_for_address) {
      etimer_set(&et, CLOCK_SECOND * 2);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      if(br_config_is_slave || border_router_is_slave_active()) {
        /* wait until no longer slave */
        continue;
      }
      send_periodic();
      if(wait_turns > 5) {
        if(uptime_elapsed(wait_started) > (10 * 60 * 1000UL)) {
          /* The border router appears to have been "waiting" for
             address more than 10 minutes. This probably means the system
             time has changed a lot and we should restart the border
             router to avoid further problems. */
          YLOG_ERROR("The system time has changed %lu minutes while RPL-border router has been waiting for address\n", uptime_elapsed(wait_started) / (60 * 1000UL));
          YLOG_ERROR("Exiting RPL-border router now!\n");
          exit(EXIT_FAILURE);
        }
      }
      if(wait_turns++ > 100) {
        YLOG_ERROR("RPL-Border router giving up waiting for address.\n");
        exit(EXIT_FAILURE);
      }
      watchdog_periodic();
    }
    YLOG_DEBUG("RPL-Border router have address\n");
  }

  if(br_config_ipaddr != NULL) {
    uip_ipaddr_t prefix;

    if(uiplib_ipaddrconv((const char *)br_config_ipaddr, &prefix)) {
      if(YLOG_IS_LEVEL(YLOG_LEVEL_INFO)) {
        YLOG_INFO("Setting prefix ");
        uip_debug_ipaddr_print(&prefix);
        PRINTA("\n");
      }
      set_prefix_64(&prefix);

    } else {
      YLOG_ERROR("Parse error: %s\n", br_config_ipaddr);
      exit(EXIT_FAILURE);
    }
  }

  if(dis > 0) {
    etimer_set(&et, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    /* send a couple of DIS and see if we get something back... */
    while(dis > 0) {
      YLOG_DEBUG("RPL - Sending DIS %d\n", dis);
      dis_output(NULL);
      etimer_set(&et, CLOCK_SECOND * 3);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      watchdog_periodic();
      dis = dis - 1;
    }
  }

  /* tun init is also responsible for setting up the SLIP connection */
  tun_init();

  if(has_dag_version) {
    /* increase if we found a version */
    RPL_LOLLIPOP_INCREMENT(dag_init_version);
    dag = rpl_set_root_with_version(RPL_DEFAULT_INSTANCE, &dag_id,
                                    dag_init_version);
  } else {
    dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &dag_id);
  }
  if(dag != NULL) {
    rpl_set_prefix(dag, &prefix, 64);
    if(has_dag_version) {
      YLOG_DEBUG("created a new RPL dag with version %u\n", dag_init_version);
    } else {
      YLOG_DEBUG("created a new RPL dag\n");
    }
  }

  print_local_addresses();

  while(1) {
    etimer_set(&et, CLOCK_SECOND * 10);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if(radio_control_version > 2) {
      border_router_request_radio_time();
    } else if(radio_control_version == 0) {
      YLOG_DEBUG("Re-request SR version\n");
      border_router_request_radio_version();
    }

    watchdog_periodic();
    /* Only send periodic information when not running as slave */
    if(!br_config_is_slave && !border_router_is_slave_active()) {
      send_periodic();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
