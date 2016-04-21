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
 */

/**
 * \file
 *         Boot data helper functions.
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef BOOT_DATA_H_
#define BOOT_DATA_H_

#include <stdint.h>

#define BOOT_DATA_MAGIC_0 0x9f
#define BOOT_DATA_MAGIC_1 0xd2
#define BOOT_DATA_MAGIC_2 0x44
#define BOOT_DATA_MAGIC_3 0xe1
#define BOOT_DATA_MAGIC_4 0x95
#define BOOT_DATA_MAGIC_5 0x2a
#define BOOT_DATA_MAGIC_6 0xca
#define BOOT_DATA_NO_SELECTED_IMAGE 0

typedef struct {
  uint8_t magic0;
  uint8_t magic1;
  uint8_t magic2;
  uint8_t magic3;
  uint8_t magic4;
  uint8_t magic5;
  uint8_t magic6;
  uint8_t next_boot_image;
  uint8_t extended_reset_cause;
  uint8_t unused_0;
  uint8_t unused_1;
  uint8_t unused_2;
  uint8_t unused_3;
  uint8_t unused_4;
  uint8_t unused_5;
  uint8_t unused_6;
} boot_data_t;

boot_data_t *boot_data_get(void);

#endif /* BOOT_DATA_H_ */
