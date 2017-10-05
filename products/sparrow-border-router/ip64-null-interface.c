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
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/slip.h"

#include "ip64.h"
#include "ip64-arp.h"
#include "ip64-eth-interface.h"

#include <string.h>

#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

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

#define UIP_IP64_BUF        ((struct uip_ip4_hdr *)ip64_packet_buffer)


/*---------------------------------------------------------------------------*/
void
ip64_null_interface_input(uint8_t *packet, uint16_t len)
{
  PRINTF("-------------->\n");
  uip_len = ip64_4to6(packet, len, &uip_buf[UIP_LLH_LEN]);
  if(uip_len > 0) {
    PRINTF("ip64_interface_process: converted %d bytes\n", uip_len);

    PRINTF("ip64-interface: input source ");
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF(" destination ");
    PRINT6ADDR(&UIP_IP_BUF->destipaddr);
    PRINTF("\n");

    tcpip_input();
    PRINTF("Done\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  PRINTF("ip64-null-interface: init\n");
}
/*---------------------------------------------------------------------------*/
static int
output(void)
{
  int len;

  PRINTF("ip64-interface: output source ");
  PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
  PRINTF(" destination ");
  PRINT6ADDR(&UIP_IP_BUF->destipaddr);
  PRINTF("\n");

  PRINTF("<--------------\n");
  len = ip64_6to4(&uip_buf[UIP_LLH_LEN], uip_len,
		  ip64_packet_buffer);

  PRINTF("ip64-interface: output len %d\n", len);
  if(len > 0) {
    PRINTF("From: %d.%d.%d.%d", UIP_IP64_BUF->srcipaddr.u8[0],
           UIP_IP64_BUF->srcipaddr.u8[1],
           UIP_IP64_BUF->srcipaddr.u8[2],
           UIP_IP64_BUF->srcipaddr.u8[3]);
    PRINTF(" To: %d.%d.%d.%d\n", UIP_IP64_BUF->destipaddr.u8[0],
           UIP_IP64_BUF->destipaddr.u8[1],
           UIP_IP64_BUF->destipaddr.u8[2],
           UIP_IP64_BUF->destipaddr.u8[3]);

    IP64_ETH_DRIVER.output(ip64_packet_buffer, len);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface ip64_null_interface = {
  init,
  output
};
/*---------------------------------------------------------------------------*/
