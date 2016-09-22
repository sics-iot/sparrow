/*
 * Copyright (c) 2015, Yanzi Networks AB.
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
 *         A NETSTACK_NETWORK proxy for Yanzi IoT Dongle to support
 *         dual operation mode.
 * \author
 *         Ioannis Glaropoulos <ioannisg@kth.se>
 */

#include "net/netstack.h"
#include <string.h>

#ifndef DUAL_MODE_SERIAL_RADIO_NETWORK_DRIVER
#define DUAL_MODE_SERIAL_RADIO_NETWORK_DRIVER encnet_driver
#endif /* DUAL_MODE_SERIAL_RADIO_NETWORK_DRIVER */

#ifndef DUAL_MODE_STANDARD_MODE_NETWORK_DRIVER
#define DUAL_MODE_STANDARD_MODE_NETWORK_DRIVER sicslowpan_driver
#endif /* DUAL_MODE_SERIAL_RADIO_NETWORK_DRIVER */

extern const struct network_driver DUAL_MODE_SERIAL_RADIO_NETWORK_DRIVER;
extern const struct network_driver DUAL_MODE_STANDARD_MODE_NETWORK_DRIVER;

/*---------------------------------------------------------------------------*/
static void
init(void)
{
  if(dual_mode_get_op_mode() == DUAL_MODE_OP_MODE_STANDARD) {
    DUAL_MODE_STANDARD_MODE_NETWORK_DRIVER.init();
  } else {
    /* Serial radio mode */
    DUAL_MODE_SERIAL_RADIO_NETWORK_DRIVER.init();
  }
}
/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
  if(dual_mode_get_op_mode() == DUAL_MODE_OP_MODE_STANDARD) {
    DUAL_MODE_STANDARD_MODE_NETWORK_DRIVER.input();
  } else {
    /* Serial radio mode */
    DUAL_MODE_SERIAL_RADIO_NETWORK_DRIVER.input();
  }
}
/*---------------------------------------------------------------------------*/
const struct network_driver dual_mode_net_driver = {
  "Dual mode net driver",
  init,
  packet_input,
};
/*---------------------------------------------------------------------------*/
