/*
 * Copyright (c) 2012-2016, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of the copyright holders nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
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
 */

/**
 * \file
 *         Sparrow helper functions.
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 */

#include "sparrow.h"
#include "sparrow-oam.h"
#include "lib/random.h"
#include "net/ip/uip.h"
#include "swrevision.h"

static uint32_t local_location_id = 0;

/*---------------------------------------------------------------------------*/
/*
 * Get 128 bit device identifier (Product ID) formatted for variable response.
 */
const sparrow_id_t *
sparrow_get_did(void)
{
  static uint8_t done = 0;
  static sparrow_id_t did =
    {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x90, 0xda, 0xff, 0xfe, 0x00, 0x4f, 0x00 }};

  if(!done) {
    int i;
    for(i = 8; i < 16; i++) {
      did.id[i] = uip_lladdr.addr[i - 8];
    }
    done = TRUE;
  }
  return &did;
}
/*---------------------------------------------------------------------------*/
uint8_t
sparrow_is_location_set(void)
{
  if(sparrow_get_location_id() == 0) {
    return FALSE;
  }
  return TRUE;
}
/*---------------------------------------------------------------------------*/
/*
 * Read location ID.
 * Return 0 on failure or when no location id has been set
 */
uint32_t
sparrow_get_location_id(void)
{

  /* TODO read location from NVM */

  return local_location_id;
}
/*---------------------------------------------------------------------------*/
/*
 * Write location ID to persistent storage.
 * Zero and 0xffffffff are not valid IDs.
 *
 * Return FALSE on failure.
 */
uint8_t
sparrow_set_location_id(uint32_t id)
{
  if(id == 0) {
    return FALSE;
  }

  if(id == 0xffffffff) {
    /* Flag to clear location id */
    local_location_id = 0;
  } else {
    local_location_id = id;
  }

  /* TODO save to NVM */

  /* If it is handled, return true */
  return TRUE;
}
/*---------------------------------------------------------------------------*/
/*
 * Read owner ID.
 * Return 0 on failure or when no owner ID has been set.
 */
uint32_t
sparrow_get_owner_id(void)
{
  /* TODO read owner id from NVM */
  return 0;
}
/*---------------------------------------------------------------------------*/
/*
 * Write owner ID.
 * Zero is not a valid id.
 *
 * Return FALSE on failure.
 */
uint8_t
sparrow_set_owner_id(uint32_t id)
{
  if(id == 0) {
    return FALSE;
  }

  /* TODO write owner id to NVM */
  return FALSE;
}
/*---------------------------------------------------------------------------*/
/*
 * Factory default
 * Sets the location id to 0
 */
uint8_t
sparrow_set_factory_default(void)
{
  uint8_t success;

  success = sparrow_set_location_id(0xffffffff);

  return success;
}
/*---------------------------------------------------------------------------*/
/*
 * Get string representation of software version
 */
const uint8_t *
sparrow_getswrevision(void)
{
  return (const uint8_t *)swrevision;
}
/*---------------------------------------------------------------------------*/
/*
 * Get string representation of compile time.
 */
const uint8_t *
sparrow_getcompiletime(void)
{
  return (const uint8_t *)compiletime;
}
/*---------------------------------------------------------------------------*/
/*
 * Write "len" bytes of random to "dst"
 */
void
sparrow_random_fill(uint8_t *dst, size_t len)
{
  int i;
  uint16_t r;
  for(i = 0; i < len; i += 2) {
    r = random_rand();
    dst[i] = (r >> 8) & 0xff;
    if((i + 1) < len) {
      dst[i + 1] = r & 0xff;
    }
  }
}
/*---------------------------------------------------------------------------*/
