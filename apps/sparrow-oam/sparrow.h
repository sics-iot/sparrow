/*
 * Copyright (c) 2012-2016, Yanzi Networks AB.
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
 *         Sparrow helper functions.
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef SPARROW_H_
#define SPARROW_H_

#include "contiki-conf.h"

/* for size_t */
#include <stddef.h>
#include "sparrow-byteorder.h"

typedef struct {
  uint8_t id[16];
} sparrow_id_t;

/*
 * Returns 128 bit product id formatted for variable response.
 */
const sparrow_id_t *sparrow_get_did(void);

uint8_t sparrow_is_location_set(void);
uint32_t sparrow_get_location_id(void);
uint8_t sparrow_set_location_id(uint32_t id);

uint32_t sparrow_get_owner_id(void);
uint8_t sparrow_set_owner_id(uint32_t id);

uint8_t sparrow_set_factory_default(void);

const uint8_t *sparrow_getswrevision(void);
const uint8_t *sparrow_getcompiletime(void);
void sparrow_random_fill(uint8_t *dst, size_t len);

/* Backwards compability for bootloader */
#include "sys/uptime.h"

static inline uint64_t
boottimer_read(void)
{
  return uptime_read();
}
static inline uint64_t
boottimer_elapsed(uint64_t when)
{
  return uptime_elapsed(when);
}
static inline uint64_t
boottimerIEEE64(void)
{
  return uptime_ieee64();
}
static inline uint32_t
boottimerIEEE64in32(void)
{
  return uptime_ieee64in32();
}

#endif /* SPARROW_H_ */
