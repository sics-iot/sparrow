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

#ifndef IMAGE_TRAILER_H_
#define IMAGE_TRAILER_H_

#include <stdint.h>

#define IMAGE_TRAILER_MAGIC 0x66745951 /* host byte order */

/* image_info must be n * 8 bytes total to avoid padding. */
typedef struct {
  uint64_t image_type;
  uint32_t start_address;
  uint32_t length;
  uint8_t number_of_sectors;
  uint8_t sectors[15];
} image_info_t;

#define BOOT_FRONT_LED_CONFIG_DO_INDICATE  0x01
#define BOOT_FRONT_LED_CONFIG_DO_CONFIG    0x02
#define BOOT_FRONT_BUTTON_READ_BUTTON      0x01
#define BOOT_FRONT_BUTTON_CONFIG_DO_CONFIG 0x02

typedef struct boot_frontpanel {
  uint8_t led_config;
  uint8_t pushbutton_config;
  uint8_t reserved0;
  uint8_t reserved1;
  uint32_t led_config_address;
  uint32_t led_config_value;
  uint32_t led_data_address;
  uint32_t led_data_value_on;
  uint32_t led_data_value_off;
  uint32_t pushbutton_config_address;
  uint32_t pushbutton_config_value;
  uint32_t pushbutton_data_address;
  uint32_t pushbutton_data_mask;
  uint32_t pushbutton_data_pushed_after_mask;
} mfg_area_boot_frontpanel_t;

#define MFG_CAPABILITY_CAN_SLEEP          0x01
#define MFG_CAPABILITY_BARBIE_RTS         0x02
#define MFG_CAPABILITY_BARBIE_MISSING_FAN 0x04

#define IMAGE_TRAILER_MFG_MAGIC0 0x01
#define IMAGE_TRAILER_MFG_MAGIC1 0x59
#define IMAGE_TRAILER_MFG_MAGIC2 0x5d
#define IMAGE_TRAILER_MFG_MAGIC3 0xe1

#define IMAGE_TRAILER_MAX_IMAGE_COUNT 2

#define MFG_EXTRA_INFO_FRONTPANEL 0x01

typedef struct {
  uint8_t revision;
  uint8_t magic1;
  uint8_t magic2;
  uint8_t magic3;
  uint8_t number_of_images;
  uint8_t extra_info;
  uint8_t reserved1;
  uint8_t reserved2;
  uint64_t capabilities;
  uint8_t mfgkey[16];
  image_info_t images[];
} mfg_area_t;

typedef struct {
  uint8_t image_type[8];
  uint8_t image_version[8];
  uint32_t image_start;
  uint8_t reserved;
  uint8_t salt;
  uint8_t algorithm;
  uint8_t length;
  uint32_t magic;
  uint32_t digest;
} image_trailer_t;

typedef enum {
  IMAGE_TRAILER_DIGEST_RESERVED = 0x00,
  IMAGE_TRAILER_DIGEST_MD5      = 0x01,
  IMAGE_TRAILER_DIGEST_SHA1     = 0x02,
  IMAGE_TRAILER_DIGEST_NONE     = 0x03, /* no digest present, just trailer */
  IMAGE_TRAILER_DIGEST_CRC32    = 0x04,
} image_trailer_digest_algorithm_t;

#define IMAGE_STATUS_OK            0x00
#define IMAGE_STATUS_BAD_CHECKSUM  0x01
#define IMAGE_STATUS_BAD_TYPE      0x02
#define IMAGE_STATUS_BAD_SIZE      0x04
#define IMAGE_STATUS_NEXT_BOOT     0x08
#define IMAGE_STATUS_WRITABLE      0x10
#define IMAGE_STATUS_ACTIVE        0x20
#define IMAGE_STATUS_ERASED        0x40

const image_trailer_t *image_trailer_find(uint8_t *start, uint32_t len, uint64_t type);
uint8_t image_trailer_verify_checksum(const image_trailer_t *T);
uint32_t image_trailer_get_image_status(const image_info_t *image);
uint32_t image_trailer_get_image_crc32(const image_info_t *image);
uint64_t image_trailer_get_image_version(const image_info_t *image);
uint32_t image_trailer_get_image_length(const image_info_t *image);
const image_info_t *image_trailer_get_images(void);
const mfg_area_t *image_trailer_get_mfg_area(void);

uint8_t image_trailer_find_mfg_area(const uint8_t *start, const uint8_t *end);

#endif /* IMAGE_TRAILER_H_ */
