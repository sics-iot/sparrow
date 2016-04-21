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

#ifndef DATAQUEUE_H_
#define DATAQUEUE_H_

#include "contiki-conf.h"
#include <stddef.h>

#ifdef DATAQUEUE_CONF_SIZE
#define DATAQUEUE_SIZE DATAQUEUE_CONF_SIZE
#else
#define DATAQUEUE_SIZE 8192
#endif

struct dataqueue {
  uint8_t data[DATAQUEUE_SIZE];
  unsigned pos, end;
};

int dataqueue_is_empty(const struct dataqueue *q);
int dataqueue_is_full(const struct dataqueue *q);
unsigned int dataqueue_size(const struct dataqueue *q);
unsigned int dataqueue_free(const struct dataqueue *q);
int dataqueue_add(struct dataqueue *q, const uint8_t *data, size_t len);
int dataqueue_read(struct dataqueue *q, int fd);
int dataqueue_write(struct dataqueue *q, int fd);
void dataqueue_clear(struct dataqueue *q);

#endif /* DATAQUEUE_H_ */
