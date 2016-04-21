/*
 * Copyright (c) 2015, SICS, Swedish ICT AB.
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
 *         Dataqueue for buffered communication
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "dataqueue.h"
#include <string.h>
#include <unistd.h>

#define YLOG_LEVEL YLOG_LEVEL_INFO
#define YLOG_NAME  "dqueue"
#include "ylog.h"

static unsigned warn_count;
/*---------------------------------------------------------------------------*/
int
dataqueue_is_empty(const struct dataqueue *q)
{
  return q->pos == q->end;
}
/*---------------------------------------------------------------------------*/
int
dataqueue_is_full(const struct dataqueue *q)
{
  return dataqueue_free(q) == 0;
}
/*---------------------------------------------------------------------------*/
unsigned int
dataqueue_free(const struct dataqueue *q)
{
  if(q->pos > q->end) {
    return q->pos - q->end - 1;
  }

  return sizeof(q->data) - q->end + q->pos - 1;
}
/*---------------------------------------------------------------------------*/
unsigned int
dataqueue_size(const struct dataqueue *q)
{
  return sizeof(q->data) - dataqueue_free(q);
}
/*---------------------------------------------------------------------------*/
int
dataqueue_add(struct dataqueue *q, const uint8_t *data, size_t len)
{
  unsigned int available;

  available = dataqueue_free(q);
  if(len > available) {
    if(warn_count++ < 50) {
      YLOG_ERROR("*** overflow (add)\n");
    }
    return 0;
  }

  if(q->pos > q->end) {
    memcpy(&q->data[q->end], data, len);
    q->end += len;
    return len;
  }

  if(len <= sizeof(q->data) - q->end) {
    memcpy(&q->data[q->end], data, len);
    q->end += len;
    if(q->end >= sizeof(q->data)) {
      q->end = 0;
    }
    return len;
  }

  memcpy(&q->data[q->end], data, sizeof(q->data) - q->end);
  memcpy(q->data, &data[sizeof(q->data) - q->end], len - sizeof(q->data) - q->end);

  q->end = len - sizeof(q->data) - q->end;
  return len;
}
/*---------------------------------------------------------------------------*/
int
dataqueue_read(struct dataqueue *q, int fd)
{
  unsigned int available;
  int len;

  if(q->pos > q->end) {
    available = q->pos - q->end - 1;
  } else {
    available = sizeof(q->data) - q->end - 1;
    if(q->pos > 0) {
      available++;
    }
  }

  if(available == 0) {
    return 0;
  }

  len = read(fd, &q->data[q->end], available);
  if(len > 0) {
    q->end += len;
    if(q->end >= sizeof(q->data)) {
      q->end = 0;
    }
  }
  return len;
}
/*---------------------------------------------------------------------------*/
int
dataqueue_write(struct dataqueue *q, int fd)
{
  int len;
  unsigned end;
  if(q->pos == q->end) {
    /* Nothing to send */
    return 0;
  }

  if(q->end > q->pos) {
    end = q->end;
  } else {
    end = sizeof(q->data);
  }

  len = write(fd, &q->data[q->pos], end - q->pos);
  if(len > 0) {
    q->pos += len;

    if(q->pos == q->end) {
      dataqueue_clear(q);
    } else if(q->pos >= sizeof(q->data)) {
      q->pos = 0;
    }
  }
  return len;
}
/*---------------------------------------------------------------------------*/
void
dataqueue_clear(struct dataqueue *q)
{
  q->pos = q->end = 0;
}
/*---------------------------------------------------------------------------*/
