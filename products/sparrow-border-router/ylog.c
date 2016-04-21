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
 *         Simple logging tool
 * \author
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"
#include "ylog.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef CONTIKI_TARGET_NATIVE
#include <time.h>
#include <sys/time.h>
#endif /* CONTIKI_TARGET_NATIVE */
/*---------------------------------------------------------------------------*/
void
ylog_format(const char *tag, const char *name, const char *message, va_list args)
{
#ifdef CONTIKI_TARGET_NATIVE
  struct timeval now;
  struct tm loctime;
  int n = 0;
  char buf[28];

  if((gettimeofday(&now, 0) == 0) &&
     (localtime_r(&now.tv_sec, &loctime) != NULL) &&
     (n = strftime(buf, sizeof(buf), "%F %T", &loctime)) > 0) {
    buf[n] = '\0';
    printf("%s.%03u [%s] %s - ", buf, (unsigned)(now.tv_usec / 1000),
           name, tag);
  } else {
    printf("- [%s] %s - ", name, tag);
  }
#else /* CONTIKI_TARGET_NATIVE */
  unsigned long time;
  time = clock_seconds();
  printf("%5lu [%s] %s - ", time, name, tag);
#endif /* CONTIKI_TARGET_NATIVE */
  vprintf(message, args);
}
/*---------------------------------------------------------------------------*/
void
ylog_error(const char *name, const char *message, ...)
{
  va_list args;
  va_start(args, message);
  ylog_format("ERROR", name, message, args);
  va_end(args);
}
/*---------------------------------------------------------------------------*/
void
ylog_info(const char *name, const char *message, ...)
{
  va_list args;
  va_start(args, message);
  ylog_format("INFO ", name, message, args);
  va_end(args);
}
/*---------------------------------------------------------------------------*/
void
ylog_debug(const char *name, const char *message, ...)
{
  va_list args;
  va_start(args, message);
  ylog_format("DEBUG", name, message, args);
  va_end(args);
}
/*---------------------------------------------------------------------------*/
void
ylog_print(const char *name, const char *message, ...)
{
  va_list args;
  va_start(args, message);
  ylog_format("     ", name, message, args);
  va_end(args);
}
/*---------------------------------------------------------------------------*/
