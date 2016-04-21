/*
 * Copyright (c) 2014-2016, SICS, Swedish ICT AB.
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
 *         A control interface via UDP for the native border router
 * \author
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"
#include "cmd.h"
#include "border-router-cmds.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_BORDER_ROUTER_CTRL

#define YLOG_LEVEL YLOG_LEVEL_DEBUG
#define YLOG_NAME  "ctrl"
#include "ylog.h"

#define PDU_MAX_LEN 1280
#define PORT 47000
#define PORT_MAX_COUNT 1000

extern const char *ctrl_config_port;

static unsigned char buf[PDU_MAX_LEN] = { 0 };
static int udp_fd = -1;

static struct sockaddr_in server;
static struct sockaddr_in client;
/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset)
{
  if(udp_fd >= 0) {
    FD_SET(udp_fd, rset);	/* Read from udp ASAP! */
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
handle_fd(fd_set *rset, fd_set *wset)
{
  int o, l;
  unsigned int a = sizeof(client);

  if(udp_fd >= 0 && FD_ISSET(udp_fd, rset)) {

    l = recvfrom(udp_fd, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&client, &a);
    if(l == -1) {
      if(errno == EAGAIN) {
        return;
      }
      YLOG_ERROR("UDP control recv failed\n");
      err(1, "border-router-ctrl: recv");
      return;
    }
    for(o = 0; o < l && buf[o] <= ' '; o++);
    for(; l - 1 > o && buf[l - 1] <= ' '; l--);
    buf[l] = '\0';
    YLOG_INFO("CONTROL: '%s'\n", &buf[o]);
    if(l > o + 1) {
      command_context = CMD_CONTEXT_STDIO;
      cmd_input(&buf[o], l - o);
    }
  }
}
/*---------------------------------------------------------------------------*/
static const struct select_callback udp_callback = { set_fd, handle_fd };
/*---------------------------------------------------------------------------*/
void
border_router_ctrl_init(void)
{
  uint16_t port = PORT;
  int i;

  if(ctrl_config_port != NULL) {
    if(strlen(ctrl_config_port) == 0 || strcmp(ctrl_config_port, "null") == 0) {
      /* UDP control is disabled */
      return;
    }

    port = atoi(ctrl_config_port);
    if(port == 0) {
      YLOG_ERROR("Illegal port '%s'. Disabling UDP control\n", ctrl_config_port);
      return;
    }
  }

  /* create a UDP socket */
  if((udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    YLOG_ERROR("Error creating control UDP socket\n");
    return;
  }

  memset((void *) &server, 0, sizeof(server));

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  /* localhost 127.0.0.1 */

  /* bind socket to port */
  for(i = 0; bind(udp_fd, (struct sockaddr*)&server, sizeof(server)) == -1; i++) {
    if(i >= PORT_MAX_COUNT) {
      YLOG_ERROR("Error binding UDP port for control\n");
      close(udp_fd);
      udp_fd = -1;
      return;
    }
    port++;
    server.sin_port = htons(port);
  }

  YLOG_INFO("Started control UDP on port %d\n", port);
  select_set_callback(udp_fd, &udp_callback);
}
#endif /* HAVE_BORDER_ROUTER_CTRL */
