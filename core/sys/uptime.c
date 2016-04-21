/*
 * Copyright (c) 2013-2016, Yanzi Networks AB.
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

/**
 * \file
 *         Uptime implementation - time since last boot
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 */

#include "sys/uptime.h"
#include "sys/clock.h"

#ifdef UPTIME_ARCH_READ
uint64_t UPTIME_ARCH_READ(void);
#endif /* UPTIME_ARCH_READ */
/*---------------------------------------------------------------------------*/
/*
 * milliseconds since boot
 */
uint64_t
uptime_read(void)
{
#ifdef UPTIME_ARCH_READ
  return UPTIME_ARCH_READ();
#elif (CLOCK_SECOND == 1000)
  return clock_time();
#else /* (CLOCK_SECOND == 1000) */
  return (uint64_t)clock_time() * 1000 / CLOCK_SECOND;
#endif /* (CLOCK_SECOND == 1000) */
}
/*---------------------------------------------------------------------------*/
/*
 * seconds since boot
 */
uint32_t
uptime_seconds(void)
{
  uint64_t now;
  now = uptime_read();
  return (uint32_t)(now / 1000);
}
/*---------------------------------------------------------------------------*/
/*
 * Give number of milliseconds that have elapsed since "when".
 */
uint64_t
uptime_elapsed(uint64_t when)
{
  /* Really simple when time can not wrap */
  return uptime_read() - when;
}
/*---------------------------------------------------------------------------*/
/*
 * Give number of milliseconds until "future".
 */
int32_t
uptime_milliseconds_until(uint64_t future)
{
  uint64_t now = uptime_read();
  uint64_t n;
  if(future <= now) {
    return 0;
  }
  n = future - now;
  if(n > 0x7fffffff) {
    return 0x7fffffff;
  }
  return (int32_t)(n & 0x7fffffff);
}
/*---------------------------------------------------------------------------*/
/*
 * IEEE 64 bit time format.
 * 32 bit seconds + 1 bit sign + 31 bits * nanoseconds.
 */
uint64_t
uptime_ieee64(void)
{
  uint64_t now;
  now = uptime_read();
  now = ((now / 1000) << 32) + ((now % 1000) * 1000000L);

#if defined(CONTIKI_TARGET_NATIVE) || defined(CONTIKI_TARGET_NATIVE_SPARROW)
  {
    /* Make sure the IEEE 64 time is always increasing on native platform */
    static uint64_t last = 0;
    if(now <= last) {
      now = last + 1;
    }
    last = now;
  }
#endif /* CONTIKI_TARGET_NATIVE || CONTIKI_TARGET_NATIVE_SPARROW */
  return now;
}
/*---------------------------------------------------------------------------*/
