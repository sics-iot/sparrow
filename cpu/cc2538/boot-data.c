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
 *         Boot data helper functions.
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 */

#include "boot-data.h"
#include "rom-util.h"
/*---------------------------------------------------------------------------*/
boot_data_t *
boot_data_get(void)
{
#ifdef HAVE_BOOT_DATA
  extern uint32_t _boot_data;
  boot_data_t *boot_data = (boot_data_t *)&_boot_data;
  uint8_t ref[7] = { BOOT_DATA_MAGIC_0, BOOT_DATA_MAGIC_1, BOOT_DATA_MAGIC_2,
                     BOOT_DATA_MAGIC_3, BOOT_DATA_MAGIC_4, BOOT_DATA_MAGIC_5,
                     BOOT_DATA_MAGIC_6 };

  if(rom_util_memcmp(ref, &boot_data->magic0, 7) != 0) {
    rom_util_memset(boot_data, 0, sizeof(boot_data_t));
    rom_util_memcpy(&boot_data->magic0, ref, 7);
  }
  return boot_data;
#else /* HAVE_BOOT_DATA */
  return NULL;
#endif /* HAVE_BOOT_DATA */
}
/*---------------------------------------------------------------------------*/
