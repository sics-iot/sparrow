/* Copyright (c) 2014, Ioannis Glaropoulos
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <stdio.h>
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define SPRINT6ADDR(addr) sprint_addr6(buf, addr)
#else
#define PRINTF(...)
#define SPRINT6ADDR(addr)
#endif
#include "resource-print-engine.h"
/*---------------------------------------------------------------------------*/
int
sprint_addr6(char *buf, const uip_ipaddr_t *addr)
{
  if(addr == NULL || addr->u8 == NULL) {
    PRINTF("(NULL IP addr)");
    return 0;
  }
  uint16_t a;
  unsigned int i;
  int f;
  uint16_t start_offset = strlen(buf);
  uint16_t offset;
  int len = 0;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
	offset = start_offset + len;
        len += sprintf(buf+offset, "::");
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
	offset = start_offset + len;
        len += sprintf(buf+offset, ":");
      }
      offset = start_offset + len;
      len += sprintf(buf+offset, "%x", a);
    }
  }
  return len;
}
/*---------------------------------------------------------------------------*/
