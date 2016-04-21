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
 *         Common image trailer functions.
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 */

#include <string.h>
#include "lib/crc32.h"
#include "image-trailer.h"
#include "byteorder.h"

#ifdef IMAGE_TRAILER_MEMCMP
int IMAGE_TRAILER_MEMCMP(const void *s1, const void *s2, size_t n);
#else
#define IMAGE_TRAILER_MEMCMP memcmp
#endif /* IMAGE_TRAILER_MEMCMP */

#define CRC32_MAGIC_REMAINDER   0x2144DF1C

#ifndef TRUE
#define TRUE  (1 == 1)
#endif /* TRUE */

#ifndef FALSE
#define FALSE (1 == 0)
#endif /*FALSE */

static const mfg_area_t *flash_mfg_area = NULL;
static const image_info_t *images = NULL;
/*----------------------------------------------------------------*/
/*
 * Search for an image trailer and between "start" + "len" and "start".
 * Return pointer to found trailer ot NULL on fail.
 */
const image_trailer_t *
image_trailer_find(uint8_t *start, uint32_t len, uint64_t type)
{
  const image_trailer_t *T;
  int i;
  uint32_t ll;
  uint8_t type_bytes[8];

  type_bytes[0] = (type >> 56) & 0xff;
  type_bytes[1] = (type >> 48) & 0xff;
  type_bytes[2] = (type >> 40) & 0xff;
  type_bytes[3] = (type >> 32) & 0xff;
  type_bytes[4] = (type >> 24) & 0xff;
  type_bytes[5] = (type >> 16) & 0xff;
  type_bytes[6] = (type >> 8) & 0xff;
  type_bytes[7] = type & 0xff;

  for(ll = len; ll > sizeof(image_trailer_t); ll -= 4) {
    T = (const image_trailer_t *)(start + ll - sizeof(image_trailer_t));
    if(T->magic != IMAGE_TRAILER_MAGIC) {
      continue;
    }

    /*
     * Have trailer.
     * We only allow one traier, so if fields don't match
     * return NULL from here on.
     */
    if(T->length < (sizeof(image_trailer_t) >> 2)) {       /* length in 32bit words */
      return NULL;
    }
    if(T->algorithm != 4) {
      return NULL;
    }

    for(i = 0; i < 8; i++) {
      if(T->image_type[i] != type_bytes[i]) {
        return NULL;
      }
    }

    if((ntohl(T->image_start) != (uint32_t)start)) {
      return NULL;
    }

    /* Hey, found one! */
    return T;
  }
  return NULL;
}
/*----------------------------------------------------------------*/
static uint8_t
check_crc32(unsigned char *buff, int len)
{
  if(CRC32_MAGIC_REMAINDER == crc32(buff, len)) {
    return 1;
  }
  return 0;
}
/*----------------------------------------------------------------*/
/*
 * Verify image checksum.
 *
 * Return 0 on fail, non-zero on success.
 */
uint8_t
image_trailer_verify_checksum(const image_trailer_t *T)
{
  uint32_t len;
  uint8_t *start;
  start = (uint8_t *)ntohl(T->image_start);
  len = (uint32_t)T + sizeof(image_trailer_t) - (uint32_t)start;
  return check_crc32(start, len);
}
/*----------------------------------------------------------------*/
/*
 * Search for a manufacturing area between "start" and "len".
 * Return TRUE if found and FALSE otherwise. The manufacturing area
 * (if found) can be retrieved using image_trailer_get_mfg_area)
 */
uint8_t
image_trailer_find_mfg_area(const uint8_t *start, const uint8_t *end)
{
  const mfg_area_t *m;
  const uint8_t *p;

  for(p = start; p < end - sizeof(mfg_area_t); p++) {
    if(*p != IMAGE_TRAILER_MFG_MAGIC0) {
      continue;
    }
    if(*(p + 1) != IMAGE_TRAILER_MFG_MAGIC1) {
      continue;
    }
    if(*(p + 2) != IMAGE_TRAILER_MFG_MAGIC2) {
      continue;
    }
    if(*(p + 3) != IMAGE_TRAILER_MFG_MAGIC3) {
      continue;
    }

    m = (const mfg_area_t *)p;
    if(m->number_of_images == 0 ||
       m->number_of_images > IMAGE_TRAILER_MAX_IMAGE_COUNT) {
      /* Too few or too many images - not a valid trailer */
      continue;
    }

    flash_mfg_area = m;
    images = flash_mfg_area->images;
    return TRUE;
  }
  return FALSE;
}
/*----------------------------------------------------------------*/
const mfg_area_t *
image_trailer_get_mfg_area(void)
{
  return flash_mfg_area;
}
/*----------------------------------------------------------------*/
const image_info_t *
image_trailer_get_images(void)
{
  return images;
}
/*----------------------------------------------------------------*/
/*
 * Get image status.
 */
uint32_t
image_trailer_get_image_status(const image_info_t *image)
{
  const image_trailer_t *T;
  uint32_t status;
  uint32_t *p;
  uint32_t erased = TRUE;
  uint8_t compare_type[8];

  compare_type[0] = (image->image_type >> 56) & 0xff;
  compare_type[1] = (image->image_type >> 48) & 0xff;
  compare_type[2] = (image->image_type >> 40) & 0xff;
  compare_type[3] = (image->image_type >> 32) & 0xff;
  compare_type[4] = (image->image_type >> 24) & 0xff;
  compare_type[5] = (image->image_type >> 16) & 0xff;
  compare_type[6] = (image->image_type >> 8) & 0xff;
  compare_type[7] = (image->image_type) & 0xff;

  for(p = (uint32_t *)image->start_address;
      (uint8_t *)p < (uint8_t *)(image->start_address + image->length);
      p++) {
    if(*p != 0xffffffff) {
      erased = FALSE;
      break;
    }
  }
  if(erased) {
    return IMAGE_STATUS_ERASED;
  }

  T = image_trailer_find((uint8_t *)image->start_address, image->length, image->image_type);
  if(T == NULL) {
    return IMAGE_STATUS_BAD_SIZE;
  }

  if(image_trailer_verify_checksum(T)) {
    status = IMAGE_STATUS_OK;
    if(IMAGE_TRAILER_MEMCMP(T->image_type, compare_type, 8) != 0) {
      status |= IMAGE_STATUS_BAD_TYPE;
    }
    if(((uint32_t)image_trailer_get_image_status > image->start_address) &&
       ((uint32_t)image_trailer_get_image_status < (image->start_address + image->length))) {
      status |= IMAGE_STATUS_ACTIVE;
    } else {
      status |= IMAGE_STATUS_WRITABLE;
    }
  } else {
    return IMAGE_STATUS_BAD_CHECKSUM;
  }
  return status;
}
/*----------------------------------------------------------------*/
uint32_t
image_trailer_get_image_crc32(const image_info_t *image)
{
  const image_trailer_t *T;
  uint8_t *start;
  uint32_t len;

  T = image_trailer_find((uint8_t *)image->start_address, image->length, image->image_type);
  if(T == NULL) {
    return 0xffffffff;
  }
  start = (uint8_t *)ntohl(T->image_start);
  len = (uint32_t)T + sizeof(image_trailer_t) - (uint32_t)start;
  return crc32(start, len);
}
/*----------------------------------------------------------------*/
/*
 * Get image version.
 *
 * Return 0 on failure.
 */
uint64_t
image_trailer_get_image_version(const image_info_t *image)
{
  const image_trailer_t *T;
  uint64_t version = 0;

  T = image_trailer_find((uint8_t *)image->start_address, image->length, image->image_type);
  if(T == NULL) {
    return 0;
  }
  version |= (uint64_t)T->image_version[0] << 56;
  version |= (uint64_t)T->image_version[1] << 48;
  version |= (uint64_t)T->image_version[2] << 40;
  version |= (uint64_t)T->image_version[3] << 32;
  version |= (uint64_t)T->image_version[4] << 24;
  version |= (uint64_t)T->image_version[5] << 16;
  version |= (uint64_t)T->image_version[6] << 8;
  version |= (uint64_t)T->image_version[7];

  return version;
}
/*----------------------------------------------------------------*/
/*
 * Get image length.
 *
 * Return maximum image length on failure.
 */
uint32_t
image_trailer_get_image_length(const image_info_t *image)
{
  const image_trailer_t *T;

  T = image_trailer_find((uint8_t *)image->start_address, image->length, image->image_type);
  if(T == NULL) {
    return image->length;
  }

  return (uint32_t)((uint8_t *)T - image->start_address + sizeof(image_trailer_t));
}
/*----------------------------------------------------------------*/
