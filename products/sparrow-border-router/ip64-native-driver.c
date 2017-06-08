/*
 * Copyright (c) 2012, Thingsquare, http://www.thingsquare.com/.
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#if CONTIKI_TARGET_NATIVE || CONTIKI_TARGET_SPARROW_NATIVE
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include "contiki-net.h"
#include <arpa/inet.h>

#include "ip64.h"
#include "ip64-driver.h"
#include "ip64-eth.h"
#include "ip64-addrmap.h"
#include "contiki.h"

#define DEBUG 1

#if DEBUG
#undef PRINTF
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

struct uip_ip4_hdr {
  /* IPV4 header */
  uint8_t vhl,
    tos,
    len[2],
    ipid[2],
    ipoffset[2],
    ttl,
    proto;
  uint16_t ipchksum;
  uip_ip4addr_t srcipaddr, destipaddr;
};


#define UIP_IP64_IPUDP_SIZE (sizeof(struct uip_ip4_hdr) + sizeof(struct uip_udp_hdr))

#define UIP_IP64_BUF        ((struct uip_ip4_hdr *)ip64_packet_buffer)
#define UIP_IP64_UDP_BUF    ((struct uip_udp_hdr *)&ip64_packet_buffer[sizeof(struct uip_ip4_hdr)])

PROCESS(ip64_native_driver_process, "IP64 tap driver");
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  process_start(&ip64_native_driver_process, NULL);
}
/*---------------------------------------------------------------------------*/
static uint16_t
poll(uint8_t *buf, const uint16_t buflen)
{
  fd_set fdset;
  struct timeval tv;
  struct ip64_addrmap_entry *entry;
  struct sockaddr_in sock_addr;
  socklen_t socklen;

  int max_fd = 0;
  int ret = 0;

  tv.tv_sec = 0;
  tv.tv_usec = 10;

  FD_ZERO(&fdset);

  entry = ip64_addrmap_list();
  while(entry != NULL) {
    FD_SET(entry->sock_fd, &fdset);
    if(entry->sock_fd > max_fd) {
      max_fd = entry->sock_fd;
    }
    entry = entry->next;
  }

  ret = select(max_fd + 1, &fdset, NULL, NULL, &tv);

  if(ret == 0) {
    return 0;
  }

  entry = ip64_addrmap_list();
  while(entry != NULL) {
    PRINTF("Checking FD for %d\n", entry->sock_fd);
    if(FD_ISSET(entry->sock_fd, &fdset)) {
      socklen = sizeof(sock_addr);
      ret = recvfrom(entry->sock_fd, &buf[UIP_IP64_IPUDP_SIZE], buflen - UIP_IP64_IPUDP_SIZE, 0,
                     (struct sockaddr *) &sock_addr, &socklen);
      if(ret == -1) {
        perror("ip64_native_poll: read\n");
      } else {
        int i;
        PRINTF("Read: %d %x -> %x %d (socklen: %d)\n",
               ret, htonl(sock_addr.sin_addr.s_addr), htonl((entry->ip4addr.u16[0] << 16) | entry->ip4addr.u16[1]),
               htons(sock_addr.sin_port), socklen);
        for(i = 0; i < ret; i++) {
          PRINTF("%c", buf[i + UIP_IP64_IPUDP_SIZE]);
        }
        uint32_t addr = htonl(sock_addr.sin_addr.s_addr);
        UIP_IP64_BUF->vhl = 0x45; /* v4, 20 bytes size */
        /* copy source address */
        UIP_IP64_BUF->srcipaddr.u8[0] = (addr >> 24) & 0xff;
        UIP_IP64_BUF->srcipaddr.u8[1] = (addr >> 16) & 0xff;
        UIP_IP64_BUF->srcipaddr.u8[2] = (addr >> 8) & 0xff;
        UIP_IP64_BUF->srcipaddr.u8[3] = (addr >> 0) & 0xff;
        /* copy dest address */
        memcpy(&UIP_IP64_BUF->destipaddr, ip64_get_hostaddr(), 4);

        UIP_IP64_BUF->proto = UIP_PROTO_UDP;

        UIP_IP64_UDP_BUF->srcport = sock_addr.sin_port;
        UIP_IP64_UDP_BUF->destport = htons(entry->mapped_port);

        /* set the payload len */
        UIP_IP64_BUF->len[0] = 0;
        UIP_IP64_BUF->len[1] = ret + UIP_IP64_IPUDP_SIZE;

        /* checksum? */

        return ret + UIP_IP64_IPUDP_SIZE;
      }
    }
    /* Go for the next entry... */
    entry = entry->next;
  }

  if(ret == -1) {
    perror("ip64_native_poll: read");
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
static int
output(uint8_t *packet, uint16_t len)
{
  struct ip64_addrmap_entry *entry;
  struct sockaddr_in destaddr;
  size_t s;

  /* This is IPv4 output */
  if(UIP_IP64_BUF->proto == UIP_PROTO_UDP) {
    PRINTF("ip64_native_send: sending %d bytes Port:%d\n", len,
           htons(UIP_IP64_UDP_BUF->srcport));
    entry = ip64_addrmap_lookup_port(htons(UIP_IP64_UDP_BUF->srcport),
                                     UIP_PROTO_UDP);
    if(entry != NULL) {
      PRINTF("Entry: mapped_port: %d\n", entry->mapped_port);
      /* Here we can send output data! */
      if(entry->sock_fd == -1) {
        /* first set it up */
        /* do we need the sock-addr? */
        memset((char *)&entry->sock_addr, 0, sizeof(entry->sock_addr));
        entry->sock_addr.sin_family = AF_INET;
        entry->sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        entry->sock_addr.sin_port = UIP_IP64_UDP_BUF->srcport;

        if((entry->sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
          perror("cannot create socket");
          return 0;
        }
        if(bind(entry->sock_fd, (struct sockaddr *)&entry->sock_addr,
                sizeof(entry->sock_addr)) < 0) {
          perror("bind failed");
          return 0;
        }
      }

      /* setup the destination address */
      memset((char *) &destaddr, 0, sizeof(destaddr));
      destaddr.sin_family = AF_INET;
      memcpy(&destaddr.sin_addr.s_addr, &UIP_IP64_BUF->destipaddr, 4);
      destaddr.sin_port = UIP_IP64_UDP_BUF->destport;

      char str[30];
      inet_ntop(AF_INET, &destaddr.sin_addr, str, 30);

      PRINTF("** Sending data to '%s':", str);
      {
        int i;
        int h = sizeof(struct uip_ip4_hdr) + sizeof(struct uip_udp_hdr);
        int l = len - h;
        for(i = 0; i < l; i++) {
          PRINTF("%02x", packet[i + h]);
        }
      }
      PRINTF("\n");

      s = sendto(entry->sock_fd,
                 &packet[sizeof(struct uip_ip4_hdr) +
                         sizeof(struct uip_udp_hdr)],
                 len - sizeof(struct uip_ip4_hdr) -
                 sizeof(struct uip_udp_hdr), 0,
                 (struct sockaddr *)&destaddr,
                 sizeof(destaddr));
      PRINTF("Sent: %d\n", (int)s);
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ip64_native_driver_process, ev, data)
{
  uint16_t len;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_PAUSE();
    len = poll(ip64_packet_buffer, ip64_packet_buffer_maxlen);
    if(len > 0) {
      IP64_INPUT(ip64_packet_buffer, len);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
const struct ip64_driver ip64_native_driver = {
  init,
  output
};

#endif /* CONTIKI_TARGET_NATIVE || CONTIKI_TARGET_SPARROW_NATIVE */
