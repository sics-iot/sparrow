/*
 * Copyright (c) 2002, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * \ingroup platform
 *
 * \defgroup sparrow_native_platform Sparrow Native platform
 *
 * Platform running in the host (Windows or Linux) environment.
 * @{
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <err.h>

#ifdef __CYGWIN__
#include "net/wpcap-drv.h"
#endif /* __CYGWIN__ */

#include "contiki.h"
#include "net/netstack.h"
#include "net/queuebuf.h"

#include "dev/serial-line.h"
#include "dev/sparrow-device.h"

#include "net/ip/uip.h"

#if NETSTACK_CONF_WITH_IPV6
#include "net/ipv6/uip-ds6.h"
#endif /* NETSTACK_CONF_WITH_IPV6 */

#ifdef SELECT_CONF_MAX
#define SELECT_MAX SELECT_CONF_MAX
#else
#define SELECT_MAX 8
#endif

static const struct select_callback *select_callback[SELECT_MAX];
static int select_max = 0;

#define MAX_TICKS (~((clock_time_t)0) / 2)

static uint8_t serial_id[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
#if !NETSTACK_CONF_WITH_IPV6
static uint16_t node_id = 0x0102;
#endif /* !NETSTACK_CONF_WITH_IPV6 */
/*---------------------------------------------------------------------------*/
int
select_set_callback(int fd, const struct select_callback *callback)
{
  int i;
  if(fd >= 0 && fd < SELECT_MAX) {
    /* Check that the callback functions are set */
    if(callback != NULL &&
       (callback->set_fd == NULL || callback->handle_fd == NULL)) {
      callback = NULL;
    }

    select_callback[fd] = callback;

    /* Update fd max */
    if(callback != NULL) {
      if(fd > select_max) {
        select_max = fd;
      }
    } else {
      select_max = 0;
      for(i = SELECT_MAX - 1; i > 0; i--) {
        if(select_callback[i] != NULL) {
          select_max = i;
          break;
        }
      }
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int is_stdin_open = 1;
static int
stdin_set_fd(fd_set *rset, fd_set *wset)
{
  if(is_stdin_open) {
    FD_SET(STDIN_FILENO, rset);
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
stdin_handle_fd(fd_set *rset, fd_set *wset)
{
  ssize_t ret;
  char c;
  if(is_stdin_open && FD_ISSET(STDIN_FILENO, rset)) {
    ret = read(STDIN_FILENO, &c, 1);
    if(ret > 0) {
      serial_line_input_byte(c);
    } else if(ret == 0) {
      /* Standard in closed */
      is_stdin_open = 0;
      fprintf(stderr, "*** stdin closed\n");
    } else if(errno != EAGAIN) {
      err(1, "stdin: read");
    }
  }
}
const static struct select_callback stdin_fd = {
  stdin_set_fd, stdin_handle_fd
};
/*---------------------------------------------------------------------------*/
static void
set_node_addr(void)
{
  linkaddr_t addr;
  int i;

  memset(&addr, 0, sizeof(linkaddr_t));
#if NETSTACK_CONF_WITH_IPV6
  memcpy(addr.u8, serial_id, sizeof(addr.u8));
#else
  if(node_id == 0) {
    for(i = 0; i < sizeof(linkaddr_t); ++i) {
      addr.u8[i] = serial_id[7 - i];
    }
  } else {
    addr.u8[0] = node_id & 0xff;
    addr.u8[1] = node_id >> 8;
  }
#endif
  linkaddr_set_node_addr(&addr);
  printf("Network started with ll address ");
  for(i = 0; i < sizeof(addr.u8) - 1; i++) {
    printf("%d.", addr.u8[i]);
  }
  printf("%d\n", addr.u8[i]);
}

/*---------------------------------------------------------------------------*/
int contiki_argc = 0;
char **contiki_argv;

int
main(int argc, char **argv)
{
  clock_init();

#if NETSTACK_CONF_WITH_IPV6
#if UIP_CONF_IPV6_RPL
  printf(CONTIKI_VERSION_STRING " started with IPV6, RPL\n");
#else
  printf(CONTIKI_VERSION_STRING " started with IPV6\n");
#endif
#else
  printf(CONTIKI_VERSION_STRING " started\n");
#endif

  /* crappy way of remembering and accessing argc/v */
  contiki_argc = argc;
  contiki_argv = argv;

  /* native under windows is hardcoded to use the first one or two args */
  /* for wpcap configuration so this needs to be "removed" from         */
  /* contiki_args (used by the native-border-router) */
#ifdef __CYGWIN__
  contiki_argc--;
  contiki_argv++;
#ifdef UIP_FALLBACK_INTERFACE
  contiki_argc--;
  contiki_argv++;
#endif
#endif

  process_init();
  process_start(&etimer_process, NULL);
  ctimer_init();
  rtimer_init();

  queuebuf_init();

#ifdef SPARROW_DEVICE
  if(SPARROW_DEVICE.init) {
    SPARROW_DEVICE.init();
  }
#endif /* SPARROW_DEVICE */

  set_node_addr();

#if NETSTACK_CONF_WITH_IPV6
  memcpy(&uip_lladdr.addr, serial_id, sizeof(uip_lladdr.addr));
#endif /* NETSTACK_CONF_WITH_IPV6 */

  netstack_init();
  printf("MAC %s RDC %s NETWORK %s\n", NETSTACK_MAC.name, NETSTACK_RDC.name, NETSTACK_NETWORK.name);

#if NETSTACK_CONF_WITH_IPV6
  process_start(&tcpip_process, NULL);
#ifdef __CYGWIN__
  process_start(&wpcap_process, NULL);
#endif /* __CYGWIN__ */
  printf("Tentative link-local IPv6 address ");
  {
    uip_ds6_addr_t *lladdr;
    int i;
    lladdr = uip_ds6_get_link_local(-1);
    for(i = 0; i < 7; ++i) {
      printf("%02x%02x:", lladdr->ipaddr.u8[i * 2],
             lladdr->ipaddr.u8[i * 2 + 1]);
    }
    /* make it hardcoded... */
    lladdr->state = ADDR_AUTOCONF;

    printf("%02x%02x\n", lladdr->ipaddr.u8[14], lladdr->ipaddr.u8[15]);
  }
#elif NETSTACK_CONF_WITH_IPV4
  process_start(&tcpip_process, NULL);
#endif /* NETSTACK_CONF_WITH_IPV6 */

  serial_line_init();

  autostart_start(autostart_processes);

  /* Make standard output unbuffered. */
  setvbuf(stdout, (char *)NULL, _IONBF, 0);

  select_set_callback(STDIN_FILENO, &stdin_fd);
  while(1) {
    fd_set fdr;
    fd_set fdw;
    int maxfd;
    int i;
    int retval;
    struct timeval tv;

    retval = process_run();

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if(!retval) {

      if(etimer_pending()) {
        clock_time_t t = etimer_next_expiration_time() - clock_time() - 1;
        if(t < MAX_TICKS) {
          tv.tv_sec = t / CLOCK_SECOND;
          tv.tv_usec = (t % CLOCK_SECOND) * 1000;
          if(tv.tv_usec == 0 && tv.tv_sec == 0) {
            /* Clock time resolution is milliseconds. Avoid millisecond
               busy loops. */
            tv.tv_usec = 250;
          }
        }
      } else {
        tv.tv_sec = 60;
      }

      /* TODO Replace the busy polling used to read data from the sio pthread */
      if(tv.tv_sec)  {
	tv.tv_sec = 0;
	tv.tv_usec = 10000;
      } else if(tv.tv_usec > 10000) {
	tv.tv_usec = 10000;
      }
    }

    FD_ZERO(&fdr);
    FD_ZERO(&fdw);
    maxfd = 0;
    for(i = 0; i <= select_max; i++) {
      if(select_callback[i] != NULL && select_callback[i]->set_fd(&fdr, &fdw)) {
        maxfd = i;
      }
    }

    retval = select(maxfd + 1, &fdr, &fdw, NULL, &tv);
    if(retval < 0) {
      if(errno != EINTR) {
        perror("select");
      }
    } else if(retval > 0) {
      /* timeout => retval == 0 */
      for(i = 0; i <= maxfd; i++) {
        if(select_callback[i] != NULL) {
          select_callback[i]->handle_fd(&fdr, &fdw);
        }
      }
    }

    if(etimer_pending() &&
       (etimer_next_expiration_time() - clock_time() - 1) > MAX_TICKS) {
      etimer_request_poll();
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
void
log_message(char *m1, char *m2)
{
  fprintf(stderr, "%s%s\n", m1, m2);
}
/*---------------------------------------------------------------------------*/
void
uip_log(char *m)
{
  fprintf(stderr, "%s\n", m);
}
/*---------------------------------------------------------------------------*/
/** @} */
