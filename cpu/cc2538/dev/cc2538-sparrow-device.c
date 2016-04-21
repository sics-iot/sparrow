/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
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
 *         CC2538 based Sparrow Device implementation
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "dev/sparrow-device.h"
#include "dev/rom-util.h"
#include "dev/sys-ctrl.h"
#include "image-trailer.h"
#include "boot-data.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define SEARCH_MFG_START 0x200000
#define SEARCH_MFG_END   0x203000

/*---------------------------------------------------------------------------*/
static void
init(void)
{
#ifdef HAVE_BOOT_DATA
  /* This will also reset any data that should be cleared between reboots. */
  boot_data_t *boot_data = boot_data_get();
  if(boot_data == NULL) {
    PRINTF("Boot data: not found\n");
  } else {
    PRINTF("Boot data: found at %p!  Ext reset cause: %u next-boot: %u\n",
           boot_data, boot_data->extended_reset_cause,
           boot_data->next_boot_image);

    /* Reset the image to boot on after next reboot */
    boot_data->next_boot_image = BOOT_DATA_NO_SELECTED_IMAGE;
  }
#endif /* HAVE_BOOT_DATA */

  if(!image_trailer_find_mfg_area((const uint8_t *)SEARCH_MFG_START,
                                  (const uint8_t *)SEARCH_MFG_END)) {
    PRINTF("Could not find mfg area\n");
  }
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_reset_cause(void)
{
  switch((REG(SYS_CTRL_CLOCK_STA) & SYS_CTRL_CLOCK_STA_RST) >> 22) {
  case 0:
    return SPARROW_DEVICE_RESET_CAUSE_POWER_FAIL;
  case 1:
    return SPARROW_DEVICE_RESET_CAUSE_HARDWARE;
  case 2:
    return SPARROW_DEVICE_RESET_CAUSE_WATCHDOG;
  case 3:
    return SPARROW_DEVICE_RESET_CAUSE_SOFTWARE;
  default:
    return SPARROW_DEVICE_RESET_CAUSE_NONE;
  }
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_extended_reset_cause(void)
{
  return SPARROW_DEVICE_EXTENDED_RESET_CAUSE_NONE;
}
/*---------------------------------------------------------------------------*/
static const char *
reset_cause(void)
{
  switch((REG(SYS_CTRL_CLOCK_STA) & SYS_CTRL_CLOCK_STA_RST) >> 22) {
  case 0:
    return "Power fail";
  case 1:
    return "External Reset";
  case 2:
    return "Watchdog Reset";
  case 3:
    return "Software Reset";
  default:
    return "Unknown";
  }
}
/*---------------------------------------------------------------------------*/
static int
get_capabilities(uint64_t *capabilities)
{
  const mfg_area_t *flash_mfg_area;
  flash_mfg_area = image_trailer_get_mfg_area();
  if(flash_mfg_area != NULL && capabilities != NULL) {
    *capabilities = flash_mfg_area->capabilities;
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static uint32_t
get_bootloader_version(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
do_reboot(void)
{
  rom_util_reset_device();
}
/*---------------------------------------------------------------------------*/
static void
reboot_to_selected_image(int image)
{
#ifdef HAVE_BOOT_DATA
  boot_data_t *data = boot_data_get();
  if(data != NULL) {
    data->next_boot_image = image & 0xff;
  }
#endif /* HAVE_BOOT_DATA */
  rom_util_reset_device();
}
/*---------------------------------------------------------------------------*/
/*
 * Return number of this running image, or -1 if unknown.
 */
static int
get_running_image(void)
{
  const mfg_area_t *flash_mfg_area;
  const image_info_t *info;
  uint32_t where = (uint32_t)get_running_image;
  int i;

  flash_mfg_area = image_trailer_get_mfg_area();
  if(flash_mfg_area == NULL) {
    return -1;
  }

  for(i = 0; i < flash_mfg_area->number_of_images; i++) {
    info = &flash_mfg_area->images[i];
    if((where > info->start_address) &&
       (where < (info->start_address + info->length))) {
      return i + 1; /* image numbers start with "1". */
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
static int
get_image_count(void)
{
  const mfg_area_t *flash_mfg_area;
  flash_mfg_area = image_trailer_get_mfg_area();
  if(flash_mfg_area == NULL) {
    return -1;
  }
  return flash_mfg_area->number_of_images;
}
/*---------------------------------------------------------------------------*/
const struct sparrow_device cc2538_sparrow_device = {
  .init = init,
  .get_capabilities = get_capabilities,
  .get_reset_cause = get_reset_cause,
  .get_extended_reset_cause = get_extended_reset_cause,
  .reset_cause = reset_cause,
  .reboot = do_reboot,
  .reboot_to_image = reboot_to_selected_image,
  .get_image_count = get_image_count,
  .get_running_image = get_running_image,
  .get_bootloader_version = get_bootloader_version
};
/*---------------------------------------------------------------------------*/
