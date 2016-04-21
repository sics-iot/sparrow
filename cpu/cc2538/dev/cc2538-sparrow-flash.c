/*
 * Copyright (c) 2015-2016, Yanzi Networks AB.
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
 *         Sparrow Flash implementation for CC2538
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 *         Ioannis Glaropoulos <ioannisg@kth.se>
 */

#include "dev/sparrow-flash.h"
#include "dev/rom-util.h"
#include "dev/watchdog.h"
#include <string.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*
 * Low level interface compatible with cc2538
 */

/* CC2538 flash contains pages of 2048 bytes length. */
#define MFB_PAGE_SIZE_B     2048
#define MFB_PAGE_MASK_B     0xFFFFF800

/*---------------------------------------------------------------------------*/
/*
 * Flash lock and unlock functions are empty in CC2538.
 * TODO Consider implementing soft lock/unlock functions instead.
 */
sparrow_flash_status_t
sparrow_flash_unlock(void)
{
  return SPARROW_FLASH_COMPLETE;
}
/*---------------------------------------------------------------------------*/
sparrow_flash_status_t
sparrow_flash_lock(void)
{
  return SPARROW_FLASH_COMPLETE;
}
/*---------------------------------------------------------------------------*/
sparrow_flash_status_t
sparrow_flash_erase_sector(uint32_t address)
{
  /* Erasing might take time */
  watchdog_periodic();

  /* Actual erase operation is performed here. */
  if(rom_util_page_erase(address & MFB_PAGE_MASK_B, MFB_PAGE_SIZE_B)) {
    /*
     * TODO map the function output to sparrow_flash_status_t return
     * values. For now we simply use the Write protection error
     */
    return SPARROW_FLASH_ERROR_WRP;
  }
  return SPARROW_FLASH_COMPLETE;
}
/*---------------------------------------------------------------------------*/
sparrow_flash_status_t
sparrow_flash_program_word(uint32_t address, uint32_t data)
{
  int32_t write_result;
  uint32_t read_data;
  sparrow_flash_status_t status = SPARROW_FLASH_COMPLETE;
  uint8_t write_count = 0;

  while(write_count < 2) {
    write_result = rom_util_program_flash(&data, address, sizeof(uint32_t));
    if(write_result) {
      /*
       * TODO map the function output to sparrow_flash_status_t return
       * values. For now we simply use the PG error flag.
       */
      PRINTF("sparrow-flash: write-err\n");
      return SPARROW_FLASH_ERROR_PG;
    }

    /* Read flash back to verify the write operation */
    memcpy(&read_data, (uint32_t *)address, sizeof(read_data));
    if(read_data == data) {
      return status;
    } else {
      PRINTF("sparrow-flash: mismatch %lx %lx %lx %lx\n",
             data, read_data, address, (*(uint32_t *)address));
      write_count++;
    }
  }
  PRINTF("sparrow-flash: verify-err\n");
  /* We couldn't verify the read for 2 consecutive times, so report an error */
  return SPARROW_FLASH_ERROR_PG;
}
/*---------------------------------------------------------------------------*/
