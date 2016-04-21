/*
 * Copyright (c) 2015-2016, SICS, Swedish ICT AB.
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
 *         A control interface via TCP for the native border router
 * \author
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"
#include "border-router.h"
#include "dataqueue.h"
#include "enc-dev.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_BORDER_ROUTER_SERVER

#define YLOG_LEVEL YLOG_LEVEL_DEBUG
#define YLOG_NAME  "server"
#include "ylog.h"

#ifndef BORDER_ROUTER_SERVER_ALIVE_TIME
#define BORDER_ROUTER_SERVER_ALIVE_TIME 180
#endif /* BORDER_ROUTER_SERVER_ALIVE_TIME */

#define PORT 9999
#define PORT_MAX_COUNT 1000

#define BUSY_MESSAGE "\0300\tBusy. Client connection already open.\r\n\0300"

static int set_fd(fd_set *rset, fd_set *wset);
static void handle_fd(fd_set *rset, fd_set *wset);

extern const char *server_config_port;

static int server_fd = -1;
static int client_fd = -1;
static uint16_t server_port;
static struct sockaddr_in client_address;

static struct dataqueue queue_to_serial;
static struct dataqueue queue_to_client;
static struct timer activity_timer;

static const struct select_callback server_callback = { set_fd, handle_fd };
/*---------------------------------------------------------------------------*/
static int
has_output(void)
{
  return !dataqueue_is_empty(&queue_to_serial);
}
/*---------------------------------------------------------------------------*/
static int
output(int fd)
{
  return dataqueue_write(&queue_to_serial, fd);
}
/*---------------------------------------------------------------------------*/
static int
input(uint8_t *buf, size_t len)
{
  return dataqueue_add(&queue_to_client, buf, len);
}
/*---------------------------------------------------------------------------*/
static const struct enc_dev_tunnel tunnel = {
  has_output,
  output,
  input
};
/*---------------------------------------------------------------------------*/
static void
close_client(int error)
{
  if(error == 0) {
    YLOG_INFO("Client connection <%d> closed\n", client_fd);
  } else {
    YLOG_ERROR("Client connection <%d> closed: %s\n", client_fd, strerror(errno));
  }

  select_set_callback(client_fd, NULL);
  enc_dev_set_tunnel(NULL);
  close(client_fd);
  client_fd = -1;
  /* Reset the radio to default settings */
  border_router_reset_radio();
  dataqueue_clear(&queue_to_serial);
  dataqueue_clear(&queue_to_client);

  /* Update the serial radio protocol version in case the master
     connection used a different version. */
  border_router_request_radio_version();
}
/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset)
{
  if(client_fd > 0) {
    if(!dataqueue_is_full(&queue_to_serial)) {
      FD_SET(client_fd, rset);
    }

    if(!dataqueue_is_empty(&queue_to_client)) {
      FD_SET(client_fd, wset);
    }
  }

  if(server_fd >= 0) {
    FD_SET(server_fd, rset);
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
handle_fd(fd_set *rset, fd_set *wset)
{
  struct sockaddr_in address;
  unsigned int address_length;
  int n, new_fd;

  if(client_fd > 0 && FD_ISSET(client_fd, rset)) {
    if(!dataqueue_is_full(&queue_to_serial)) {
      n = dataqueue_read(&queue_to_serial, client_fd);
      if(n == 0) {
        /* Nothing read but queue is not full. Connection has closed. */
        close_client(n);
      } else if(n < 0) {
        if(errno != EAGAIN) {
          close_client(n);
        }
      } else {
        timer_restart(&activity_timer);
      }
    }
  }

  if(client_fd > 0 && FD_ISSET(client_fd, wset) && !dataqueue_is_empty(&queue_to_client)) {
    n = dataqueue_write(&queue_to_client, client_fd);
    if(n == 0) {
      close_client(n);
    } else if(n < 0) {
      if(errno != EINTR && errno != EAGAIN) {
        err(1, "br-server: read");
      }
    }
  }

  if(server_fd >= 0 && FD_ISSET(server_fd, rset)) {
    address_length = sizeof(address);
    new_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&address_length);
    if(new_fd < 0) {
      YLOG_ERROR("*** failed to accept new connection: %s\n", strerror(errno));
    } else if(new_fd > 0) {
#if ((YLOG_LEVEL) & YLOG_LEVEL_INFO) == YLOG_LEVEL_INFO
      const char *host = inet_ntoa(address.sin_addr);
      YLOG_INFO("New connection <%d> from %s, port %d\n", new_fd, host == NULL ? "" : host,
                ntohs(address.sin_port));
#endif /* ((YLOG_LEVEL) & YLOG_LEVEL_INFO) == YLOG_LEVEL_INFO */
      fcntl(new_fd, F_SETFL, O_NONBLOCK);

      if(client_fd > 0) {
        /* There already is a client connection */

        if(address_length >= sizeof(client_address) && address.sin_family == AF_INET &&
           address.sin_addr.s_addr == client_address.sin_addr.s_addr) {
          /* Connection from same host that current connection. Switch to the new connection. */
          YLOG_INFO("Old connection <%d> is from same host. Switching to new connection <%d>\n",
                    client_fd, new_fd);
          close_client(0);
        } else if(timer_expired(&activity_timer)) {
          /* The existing connection has been idle for a long time. Switch to the new connection. */
          YLOG_INFO("Old connection <%d> is inactive. Switching to new connection <%d>.\n",
                    client_fd, new_fd);
          close_client(0);
        }
      }

      if(client_fd < 0) {
        client_fd = new_fd;
        memcpy(&client_address, &address, sizeof(client_address));
        dataqueue_clear(&queue_to_serial);
        dataqueue_clear(&queue_to_client);
        select_set_callback(client_fd, &server_callback);
        enc_dev_set_tunnel(&tunnel);
        timer_set(&activity_timer, BORDER_ROUTER_SERVER_ALIVE_TIME * CLOCK_SECOND);
      } else {
        /* Deny the new connection */
        YLOG_ERROR("Client connection already open! Ignoring new connection (%lu sec left)\n",
                   (unsigned long)(timer_remaining(&activity_timer) / CLOCK_SECOND));

        send(new_fd, BUSY_MESSAGE, sizeof(BUSY_MESSAGE) - 1, 0);
        close(new_fd);
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
int
border_router_server_has_client_connection(void)
{
  return client_fd > 0 && !timer_expired(&activity_timer);
}
/*---------------------------------------------------------------------------*/
uint16_t
border_router_server_get_port(void)
{
  return server_port;
}
/*---------------------------------------------------------------------------*/
void
border_router_server_init(void)
{
  struct sockaddr_in address;
  int i;

  server_port = PORT;

  if(server_config_port != NULL) {
    if(strlen(server_config_port) == 0 || strcmp(server_config_port, "null") == 0) {
      /* Server is disabled */
      return;
    }

    server_port = atoi(server_config_port);
    if(server_port == 0) {
      YLOG_ERROR("Illegal port '%s'. Disabling server functionality.\n", server_config_port);
      return;
    }
  }

  /* create a UDP socket */
  if((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    YLOG_ERROR("Error creating server socket: %s\n", strerror(errno));
    return;
  }

  memset((void *)&address, 0, sizeof(address));

  address.sin_family = AF_INET;
  address.sin_port = htons(server_port);
  if(border_router_is_slave()) {
    /* Global access in slave mode */
    address.sin_addr.s_addr = INADDR_ANY;
  } else {
    /* Only access from localhost 127.0.0.1 */
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  }

  /* bind socket to port */
  for(i = 0; bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1; i++) {
    if(i >= PORT_MAX_COUNT) {
      YLOG_ERROR("Error binding port for server\n");
      close(server_fd);
      server_fd = -1;
      return;
    }
    server_port++;
    address.sin_port = htons(server_port);
  }

  if(listen(server_fd, 3) < 0) {
    YLOG_ERROR("Error configuring port for server: %s\n", strerror(errno));
    close(server_fd);
    server_fd = -1;
    return;
  }

  fcntl(server_fd, F_SETFL, O_NONBLOCK);

  YLOG_INFO("Started server on port %d%s\n", server_port,
            border_router_is_slave() ? " (GLOBAL ACCESS)" : "");
  select_set_callback(server_fd, &server_callback);
}
#endif /* HAVE_BORDER_ROUTER_SERVER */
