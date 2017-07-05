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
 *         Sparrow Device API
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef SPARROW_DEVICE_H_
#define SPARROW_DEVICE_H_

#include "contiki-conf.h"

typedef uint8_t sparrow_device_reset_cause_t;
typedef uint8_t sparrow_device_ext_reset_cause_t;

enum {
  SPARROW_DEVICE_RESET_CAUSE_NONE       = 0,
  SPARROW_DEVICE_RESET_CAUSE_LOCKUP     = 1,
  SPARROW_DEVICE_RESET_CAUSE_DEEP_SLEEP = 2,
  SPARROW_DEVICE_RESET_CAUSE_SOFTWARE   = 3,
  SPARROW_DEVICE_RESET_CAUSE_WATCHDOG   = 4,
  SPARROW_DEVICE_RESET_CAUSE_HARDWARE   = 5,
  SPARROW_DEVICE_RESET_CAUSE_POWER_FAIL = 6,
  SPARROW_DEVICE_RESET_CAUSE_COLD_START = 7
};

enum {
  SPARROW_DEVICE_EXTENDED_RESET_CAUSE_NONE     = 0,
  SPARROW_DEVICE_EXTENDED_RESET_CAUSE_NORMAL   = 1,
  SPARROW_DEVICE_EXTENDED_RESET_CAUSE_WATCHDOG = 2
};

struct sparrow_device {
  void (* init)(void);

  int (* get_capabilities)(uint64_t *capabilities);

  sparrow_device_reset_cause_t (* get_reset_cause)(void);
  sparrow_device_ext_reset_cause_t (* get_extended_reset_cause)(void);
  const char *(* reset_cause)(void);

  /*
   * Beep or use visual indication to identify the device chassis.
   */
  void (* beep)(uint32_t mode);

  void (* reboot)(void);

  void (* reboot_to_image)(int image);
  int (* get_image_count)(void);
  int (* get_running_image)(void);
  uint32_t (* get_bootloader_version)(void);
};

#ifdef SPARROW_DEVICE
extern const struct sparrow_device SPARROW_DEVICE;
#endif /* SPARROW_DEVICE */

#endif /* SPARROW_DEVICE_H_ */
