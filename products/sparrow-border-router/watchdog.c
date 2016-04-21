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
 *         Watchdog supervising both system time and activity
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "dev/watchdog.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#if CLOCK_USE_CLOCK_GETTIME
#include <time.h>
#else /* CLOCK_USE_CLOCK_GETTIME */
#include <sys/time.h>
#endif /* CLOCK_USE_CLOCK_GETTIME */

#define YLOG_LEVEL YLOG_LEVEL_INFO
#define YLOG_NAME  "watchdog"
#include "ylog.h"

#if CLOCK_USE_CLOCK_GETTIME || CLOCK_USE_RELATIVE_TIME
/* No need to check for host system time changes */
#define CHECK_FOR_TIME_CHANGES 0
#else
#define CHECK_FOR_TIME_CHANGES 1
#endif

#define CHECK_PERIOD         10
#define GUARD_TIME_FORWARD  (5 * 60UL)
#define GUARD_TIME_BACKWARD (1 * 60UL)
#define PERIODIC_TIME       (6 * 60UL)

static pthread_t thread;
static int thread_running = 0;
static volatile int periodic_running = 0;
static volatile int periodic_check = 0;
/*---------------------------------------------------------------------------*/
static unsigned long
system_time(void)
{
  /* Use same host clock as the native Contiki clock is using */

#if CLOCK_USE_CLOCK_GETTIME

  struct timespec tp;

  if(clock_gettime(CLOCK_MONOTONIC, &tp)) {
    YLOG_ERROR("watchdog failed to retrieve system time\n");
    exit(EXIT_FAILURE);
    return 0;
  }

  return tp.tv_sec;

#else /* CLOCK_USE_CLOCK_GETTIME */

  struct timeval tv;

  if(gettimeofday(&tv, NULL)) {
    YLOG_ERROR("watchdog failed to retrieve system time\n");
    exit(EXIT_FAILURE);
    return 0;
  }
  return tv.tv_sec;

#endif /* CLOCK_USE_CLOCK_GETTIME */
}
/*---------------------------------------------------------------------------*/
static void *
watchdog_thread(void *argument)
{
  static unsigned long last;
  static unsigned inactivity = 0;
  static unsigned no_sleep_counter = 0;
  static unsigned same_time_counter = 0;
  unsigned long t0;

  YLOG_INFO("Watchdog started!\n");

  last = system_time();
  while(1) {

    if(sleep(CHECK_PERIOD) == CHECK_PERIOD) {
      YLOG_DEBUG("did not sleep\n");
      no_sleep_counter++;
      if(no_sleep_counter > 1000) {
        YLOG_ERROR("not sleeping!\n");
        exit(EXIT_FAILURE);
      }
      continue;
    }
    no_sleep_counter = 0;

    t0 = system_time();

    YLOG_DEBUG("watchdog check after %lu sec (%lu - %lu)\n",
               t0 - last, t0, last);

    if(periodic_check) {
      inactivity = 0;
      periodic_check = 0;

    } else if(periodic_running) {
      inactivity++;

      if(inactivity > (PERIODIC_TIME / CHECK_PERIOD)) {
        /* Time has changed too much since last check */
        YLOG_ERROR("watchdog trigged\n");
        exit(EXIT_FAILURE);
      }
    }

    if(t0 == last) {
      /* Time is not moving forward */
      YLOG_INFO("system time has not changed since last check!\n");
      same_time_counter++;
      if(same_time_counter > 10) {
        YLOG_ERROR("system time not changing!\n");
        exit(EXIT_FAILURE);
      }
      continue;
    }
    same_time_counter = 0;

#if CHECK_FOR_TIME_CHANGES
    if(t0 < last) {
      /* Time has moved backwards */
      YLOG_INFO("system time has moved backwards %lu sec (%lu < %lu)\n",
                last - t0, t0, last);
      if((last - t0) > GUARD_TIME_BACKWARD) {
        YLOG_ERROR("system time has moved backwards\n");
        exit(EXIT_FAILURE);
      }
      last = t0;
      continue;
    }

    if((t0 - last) > GUARD_TIME_FORWARD) {
      /* Time has changed too much since last check */
      YLOG_ERROR("system time moved forward %lu seconds (%lu > %lu)\n",
                 t0 - last, t0, last);
      exit(EXIT_FAILURE);
    }
#endif /* CHECK_FOR_TIME_CHANGES */

    last = t0;
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
void
watchdog_init(void)
{
  int rc;
  if(thread_running) {
    /* Already running */
    return;
  }
  rc = pthread_create(&thread, NULL, watchdog_thread, NULL);
  if(rc != 0) {
    YLOG_ERROR("failed to start the watchdog thread: %d!\n", rc);
  } else {
    YLOG_DEBUG("created watchdog thread\n");
    thread_running = 1;
  }
}
/*---------------------------------------------------------------------------*/
void
watchdog_start(void)
{
  periodic_check = 1;
  periodic_running = 1;
}
/*---------------------------------------------------------------------------*/
void
watchdog_stop(void)
{
  periodic_running = 0;
}
/*---------------------------------------------------------------------------*/
void
watchdog_periodic(void)
{
  int was = periodic_check;
  periodic_check = 1;
  if(!was) {
    YLOG_DEBUG("watchdog periodic\n");
  }
}
/*---------------------------------------------------------------------------*/
void
watchdog_reboot(void)
{
  /* Death by watchdog. */
  YLOG_INFO("watchdog reboot\n");
  exit(EXIT_FAILURE);
}
/*---------------------------------------------------------------------------*/
