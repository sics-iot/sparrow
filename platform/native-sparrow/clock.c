/*
 * Copyright (c) 2005, Swedish Institute of Computer Science.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Clock implementation for Unix.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "sys/clock.h"
#include <unistd.h>

#ifndef CLOCK_USE_CLOCK_GETTIME
#define CLOCK_USE_CLOCK_GETTIME 0
#endif

#ifndef CLOCK_USE_RELATIVE_TIME
#define CLOCK_USE_RELATIVE_TIME 0
#endif

#if CLOCK_USE_CLOCK_GETTIME
#include <time.h>
#else
#include <sys/time.h>
#endif

/* The maximal time allowed to move forward between updates */
#define MAX_TIME_CHANGE_MSEC 60000UL

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if CLOCK_USE_RELATIVE_TIME
static clock_time_t uptime_msec = 0;
static unsigned long uptime_sec = 0;
#endif /* CLOCK_USE_RELATIVE_TIME */

static clock_time_t last_msec;
static unsigned long last_sec;
/*---------------------------------------------------------------------------*/
static int
get_system_time(clock_time_t *t, unsigned long *s)
{
#if CLOCK_USE_CLOCK_GETTIME

  struct timespec tp;

  if(clock_gettime(CLOCK_MONOTONIC, &tp)) {
    return 1;
  }

  if(t) {
    *t = tp.tv_sec * (clock_time_t)1000;
    *t += tp.tv_nsec / (clock_time_t)1000000;
  }
  if(s) {
    *s = tp.tv_sec;
  }

#else /* CLOCK_USE_CLOCK_GETTIME */

  struct timeval tv;

  if(gettimeofday(&tv, NULL)) {
    return 1;
  }

  if(t) {
    *t = tv.tv_sec * (clock_time_t)1000;
    *t += tv.tv_usec / (clock_time_t)1000;
  }
  if(s) {
    *s = tv.tv_sec;
  }

#endif /* CLOCK_USE_CLOCK_GETTIME */

  return 0;
}
/*---------------------------------------------------------------------------*/
#if CLOCK_USE_RELATIVE_TIME
static void
update_time(void)
{
  clock_time_t t;
  unsigned long s;

  if(get_system_time(&t, &s)) {
    /* Use the last update time */
    PRINTF("*** failed to retrieve system time\n");
    return;
  }

  if(t < last_msec) {
    /* System time has moved backwards */
    PRINTF("*** system time has moved backwards %lu msec\n",
           (unsigned long)(last_msec - t));

  } else if(t - last_msec > MAX_TIME_CHANGE_MSEC) {
    /* Too large jump forward in system time */
    PRINTF("*** system time has moved forward %lu msec\n",
           (unsigned long)(t - last_msec));
    uptime_msec += 1000UL;
    uptime_sec += 1UL;

  } else {
    uptime_msec += t - last_msec;
    uptime_sec += s - last_sec;

  }

  last_msec = t;
  last_sec = s;
}
#endif /* CLOCK_USE_RELATIVE_TIME */
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  get_system_time(&last_msec, &last_sec);
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
#if CLOCK_USE_RELATIVE_TIME

  update_time();
  return uptime_msec;

#else /* CLOCK_USE_RELATIVE_TIME */

  clock_time_t t;

  if(get_system_time(&t, NULL)) {
    PRINTF("*** failed to retrieve system time\n");
    return 0;
  }

  if(t < last_msec) {
    /* Time has moved backwards */
    return 0;
  }
  return t - last_msec;

#endif /* CLOCK_USE_RELATIVE_TIME */
}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
#if CLOCK_USE_RELATIVE_TIME

  update_time();
  return uptime_sec;

#else /* CLOCK_USE_RELATIVE_TIME */

  unsigned long s;

  if(get_system_time(NULL, &s)) {
    PRINTF("*** failed to retrieve system time\n");
    return 0;
  }

  if(s < last_sec) {
    /* Time has moved backwards */
    return 0;
  }
  return s - last_sec;

#endif /* CLOCK_USE_RELATIVE_TIME */
}
/*---------------------------------------------------------------------------*/
/**
 * Wait for a multiple of clock ticks.
 */
void
clock_wait(clock_time_t wait)
{
  clock_time_t start;

  start = clock_time();
  while(clock_time() - start < wait) {
    usleep(1000);
  }
}
/*---------------------------------------------------------------------------*/
void
clock_delay_usec(uint16_t us)
{
  usleep(us);
}
/*---------------------------------------------------------------------------*/
void
clock_delay(unsigned int us)
{
  usleep(us);
}
/*---------------------------------------------------------------------------*/
