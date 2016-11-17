/*
 * Copyright (c) 2015, Yanzi Networks AB.
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
 *    This product includes software developed by Yanzi Networks AB.
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

#include "contiki.h"
#include "net/llsec/llsec802154.h"
#include "usb/usb-serial.h"
#include "serial-radio.h"
#include "net/llsec/noncoresec/noncoresec.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

PROCESS_NAME(serial_radio_process);
PROCESS(dual_mode, "dual mode");
AUTOSTART_PROCESSES(&dual_mode);

uint8_t *USB_SERIAL_RADIO_GET_PRODUCT_DESCRIPTION(void);

struct product {
  uint8_t size;
  uint8_t type;
  uint16_t string[12];
};

/* Use generic product name */
static const struct product product = {
  sizeof(product),
  3,
  {
    'S','p','a','r','r','o','w',' ','N','o','d','e'
  }
};

uint8_t *
dual_mode_get_product_description(void)
{
  if(dual_mode_get_op_mode() == DUAL_MODE_OP_MODE_STANDARD) {
    return (uint8_t *)&product;
  }
  return USB_SERIAL_RADIO_GET_PRODUCT_DESCRIPTION();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dual_mode, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  if(dual_mode_get_op_mode() == DUAL_MODE_OP_MODE_STANDARD) {
    while(1) {
      etimer_set(&timer, CLOCK_SECOND * 5);
      PROCESS_WAIT_EVENT();
    }
  } else {
    /* Serial radio mode */

#if 0 /* FIXME: missing run time function for setting LLSEC security level */
    /* Disable encryption in serial radio - controlled by border router */
    noncoresec_set_security_level(0);
#endif

#if SLIP_ARCH_CONF_USB || DBG_CONF_USB
    /* No USB write timeout when running as serial radio */
    usb_serial_set_write_timeout(0);
#endif

    process_start(&serial_radio_process, NULL);
  }
  PROCESS_END();
}
