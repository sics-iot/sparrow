/*
 * Copyright (c) 2013-2016, SICS, Swedish ICT AB.
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
 *         Platform API
 * \author
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"
#include "dev/sparrow-device.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  /* Nothing to initialize */
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_reset_cause(void)
{
  return SPARROW_DEVICE_RESET_CAUSE_NONE;
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
  return "Unknown";
}
/*---------------------------------------------------------------------------*/
static int
get_capabilities(uint64_t *capabilities)
{
  if(capabilities) {
    *capabilities = 0ULL;
  }
  return 1;
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
  printf("*** Reboot called\n");
}
/*---------------------------------------------------------------------------*/
static void
reboot_to_selected_image(int image)
{
  printf("*** Reboot to image %u called\n", image);
}
/*---------------------------------------------------------------------------*/
/*
 * Return number of this running image, or -1 if unknown.
 */
static int
get_running_image(void)
{
  return -1;
}
/*---------------------------------------------------------------------------*/
static int
get_image_count(void)
{
  return -1;
}
/*---------------------------------------------------------------------------*/
const struct sparrow_device native_sparrow_device = {
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
