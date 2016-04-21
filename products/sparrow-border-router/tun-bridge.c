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
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "net/netstack.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#define YLOG_LEVEL YLOG_LEVEL_INFO
#define YLOG_NAME  "tun"
#include "ylog.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#ifdef linux
#include <linux/if.h>
#include <linux/if_tun.h>
#endif

#if LATENCY_STATISTICS
#include "latency-stats.h"
#endif

#include <err.h>
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "cmd.h"
#include "border-router.h"
#include "br-config.h"

static int is_open = 0;

#ifndef __CYGWIN__
static int tunfd;

static int set_fd(fd_set *rset, fd_set *wset);
static void handle_fd(fd_set *rset, fd_set *wset);
static const struct select_callback tun_select_callback = {
  set_fd,
  handle_fd
};
#endif /* __CYGWIN__ */

static int ssystem(const char *fmt, ...)
     __attribute__((__format__ (__printf__, 1, 2)));

static int
ssystem(const char *fmt, ...)
{
  char cmd[128];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(cmd, sizeof(cmd), fmt, ap);
  va_end(ap);
  YLOG_DEBUG("%s\n", cmd);
  fflush(stdout);
  return system(cmd);
}

/*---------------------------------------------------------------------------*/
static void
cleanup(void)
{
  is_open = 0;
#ifdef __APPLE__
  ssystem("ifconfig %s inet6 %s remove", br_config_tundev, br_config_ipaddr);
  ssystem("ifconfig %s down", br_config_tundev);
  ssystem("sysctl -w net.inet6.ip6.forwarding=0");
#else /* ! __APPLE__ */
  ssystem("ifconfig %s down", br_config_tundev);
#ifndef linux
  ssystem("sysctl -w net.inet.ip.forwarding=0");
    /* ssystem("sysctl -w net.ipv6.conf.all.forwarding=0"); */
#endif
  ssystem("netstat -nr"
          " | awk '{ if ($2 == \"%s\") print \"route delete -net \"$1; }'"
          " | sh",
          br_config_tundev);
#endif /* ! __APPLE__ */
}

/*---------------------------------------------------------------------------*/
static void
sigcleanup(int signo)
{
  YLOG_ERROR("signal %d\n", signo);
  exit(EXIT_FAILURE);              /* exit() will call cleanup() */
}

/*---------------------------------------------------------------------------*/
static void
ifconf(const char *tundev, const char *ipaddr)
{
#ifdef linux
  /* ssystem("ifconfig %s inet `hostname` up", tundev); */
  ssystem("ifconfig %s up", tundev);
  ssystem("ifconfig %s add %s", tundev, ipaddr);
#elif defined(__APPLE__)
  ssystem("ifconfig %s inet6 up", tundev);
  ssystem("ifconfig %s inet6 %s add", tundev, ipaddr);
  ssystem("sysctl -w net.inet6.ip6.forwarding=1");
#else
  ssystem("ifconfig %s inet `hostname` %s up", tundev, ipaddr);
  ssystem("sysctl -w net.inet.ip.forwarding=1");
#endif /* !linux */

  /* Print the configuration to the console. */
  ssystem("ifconfig %s", tundev);
}
/*---------------------------------------------------------------------------*/
int
devopen(const char *dev, int flags)
{
  char t[1024];
  strcpy(t, "/dev/");
  strncat(t, dev, sizeof(t) - 1 - 5);
  return open(t, flags);
}
/*---------------------------------------------------------------------------*/
#ifdef linux
static int
tun_alloc(char *dev)
{
  struct ifreq ifr;
  int fd, err;

  if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_NO_PI - Do not provide packet information
   */
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  if(*dev != 0) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
    close(fd);
    return err;
  }
  strcpy(dev, ifr.ifr_name);
  return fd;
}
#else
static int
tun_alloc(char *dev)
{
  return devopen(dev, O_RDWR);
}
#endif

#ifdef __CYGWIN__
/*wpcap process is used to connect to host interface */
void
tun_init(void)
{
  setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */
}

#else

static uint16_t delaymsec = 0;
static uint32_t delaystartsec, delaystartmsec;

/*---------------------------------------------------------------------------*/
void
tun_init(void)
{
  setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

  tunfd = tun_alloc(br_config_tundev);
  if(tunfd == -1) {
    err(1, "failed to allocate tun device ``%s''", br_config_tundev);
  }

  select_set_callback(tunfd, &tun_select_callback);

  YLOG_INFO("opened tun device ``/dev/%s''\n", br_config_tundev);

  atexit(cleanup);
  signal(SIGHUP, sigcleanup);
  signal(SIGTERM, sigcleanup);
  signal(SIGINT, sigcleanup);
  ifconf(br_config_tundev, br_config_ipaddr);
  is_open = 1;
}

/*---------------------------------------------------------------------------*/
static int
tun_output(uint8_t *data, int len)
{
  if(is_open && write(tunfd, data, len) != len) {
    err(1, "serial_to_tun: write");
    return -1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
tun_input(uint8_t *data, int maxlen)
{
  int size = 0;
  if(is_open) {
    if((size = read(tunfd, data, maxlen)) == -1) {
      err(1, "tun_input: read");
    }
  }
  return size;
}

/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
static int
output(void)
{
  PRINTF("SUT: %u\n", uip_len);
  if(uip_len > 0) {
#if LATENCY_STATISTICS
    latency_stats_handle_packet(&uip_buf[UIP_LLH_LEN], uip_len, LATENCY_STATISTICS_FROM_PAN);
#endif /* LATENCY_STATISTICS */
    return tun_output(&uip_buf[UIP_LLH_LEN], uip_len);
  }
  return 0;
}


const struct uip_fallback_interface rpl_interface = {
  init, output
};

/*---------------------------------------------------------------------------*/
/* tun and slip select callback                                              */
/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset)
{
  if(is_open) {
    FD_SET(tunfd, rset);
  }
  return 1;
}

/*---------------------------------------------------------------------------*/

static void
handle_fd(fd_set *rset, fd_set *wset)
{
  /* Optional delay between outgoing packets */
  /* Base delay times number of 6lowpan fragments to be sent */
  /* delaymsec = 10; */
  if(delaymsec) {
    struct timeval tv;
    int dmsec;
    gettimeofday(&tv, NULL);
    dmsec = (tv.tv_sec - delaystartsec) * 1000 + tv.tv_usec / 1000 - delaystartmsec;
    if(dmsec < 0) {
      delaymsec = 0;
    }
    if(dmsec > delaymsec) {
      delaymsec=0;
    }
  }

  if(delaymsec == 0 && is_open) {
    int size;

    if(FD_ISSET(tunfd, rset)) {
      size = tun_input(&uip_buf[UIP_LLH_LEN], sizeof(uip_buf));
      uip_len = size;

      /* Incoming packet from TUN */
#if LATENCY_STATISTICS
      latency_stats_handle_packet(&uip_buf[UIP_LLH_LEN], size, LATENCY_STATISTICS_TO_PAN);
#endif

      PRINTF("TUN data incoming read:%d PROCESS\n", size);
      tcpip_input();

      if(br_config_basedelay) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        delaymsec = br_config_basedelay;
        delaystartsec = tv.tv_sec;
        delaystartmsec = tv.tv_usec / 1000;
      }
    }
  }
}
#endif /*  __CYGWIN_ */
/*---------------------------------------------------------------------------*/
