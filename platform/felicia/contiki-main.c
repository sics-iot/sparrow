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

/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \addtogroup platform
 * @{
 *
 * \defgroup felicia The felicia module
 *
 * The felicia is a module from Yanzi Networks AB, based on the
 * cc2530 SoC with an ARM Cortex-M3 core.
 * @{
 *
 * \file
 *   Main module for the felicia based platforms
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/leds.h"
#include "dev/sys-ctrl.h"
#include "dev/scb.h"
#include "dev/nvic.h"
#include "dev/uart.h"
#include "dev/watchdog.h"
#include "dev/ioc.h"
#include "dev/serial-line.h"
#include "dev/sparrow-device.h"
#include "dev/slip.h"
#include "dev/cc2538-rf.h"
#include "dev/udma.h"
#include "dev/crypto.h"
#include "usb/usb-serial.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/queuebuf.h"
#include "net/ip/tcpip.h"
#include "net/ip/uip.h"
#include "net/mac/frame802154.h"
#include "cpu.h"
#include "reg.h"
#include "ieee-addr.h"
#include "lpm.h"
#include "dev/adc.h"
#if PLATFORM_HAS_SENSORS
#include "lib/sensors.h"
#endif /* PLATFORM_HAS_SENSORS */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
#ifdef MY_APP_PROCESS
PROCESS_NAME(MY_APP_PROCESS);
#endif /* MY_APP_PROCESS */
#ifdef MY_APP_INIT_FUNCTION
void MY_APP_INIT_FUNCTION(void);
#endif /* MY_APP_INIT_FUNCTION */
/*---------------------------------------------------------------------------*/
#if STARTUP_CONF_VERBOSE
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
#if UART_CONF_ENABLE
#define PUTS(s) puts(s)
#else
#define PUTS(s)
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAVE_SPARROW_OAM
#include "sparrow-oam.h"
#endif /* HAVE_SPARROW_OAM */
#ifdef HAVE_NETSCAN
#include "netscan.h"
#endif /* HAVE_NETSCAN */

#if PLATFORM_WITH_DUAL_MODE == 1
/* The state of the operation mode is stored in contiki-main */
static dual_mode_op_mode_t op_mode = DUAL_MODE_OP_MODE_STANDARD;
/*---------------------------------------------------------------------------*/
/*
 * Return the current operation mode of the mote.
 */
dual_mode_op_mode_t
dual_mode_get_op_mode(void)
{
  return op_mode;
}
/*---------------------------------------------------------------------------*/
#endif /* PLATFORM_WITH_DUAL_MODE */
/*---------------------------------------------------------------------------*/
/** \brief Board specific iniatialisation */
void board_init(void);
/*---------------------------------------------------------------------------*/
static void
set_rf_params(void)
{
  uint16_t short_addr;
  uint8_t ext_addr[8];

  ieee_addr_cpy_to(ext_addr, 8);

  short_addr = ext_addr[7];
  short_addr |= ext_addr[6] << 8;

  /* Populate linkaddr_node_addr. Maintain endianness */
  memcpy(&linkaddr_node_addr, &ext_addr[8 - LINKADDR_SIZE], LINKADDR_SIZE);

#if STARTUP_CONF_VERBOSE
  {
    int i;
    printf("Network configured with ll address ");
    for(i = 0; i < LINKADDR_SIZE - 1; i++) {
      printf("%02x:", linkaddr_node_addr.u8[i]);
    }
    printf("%02x\n", linkaddr_node_addr.u8[i]);
  }
#endif

  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, CC2538_RF_CHANNEL);
  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, 8);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Main routine for the felicia platform
 */
int
main(void)
{
#if PLATFORM_WITH_DUAL_MODE && USB_SERIAL_CONF_ENABLE
  struct timer detect_usb_timer;
#endif

  nvic_init();
  ioc_init();
  sys_ctrl_init();
  clock_init();
  lpm_init();
  rtimer_init();
  gpio_init();

#if PLATFORM_HAS_LEDS
  leds_init();
#endif /* PLATFORM_HAS_LEDS */

  process_init();
  watchdog_init();
  board_init();

  /*
   * Character I/O Initialisation.
   * When the UART receives a character it will call serial_line_input_byte to
   * notify the core. The same applies for the USB driver.
   *
   * If slip-arch is also linked in afterwards (e.g. if we are a border router)
   * it will overwrite one of the two peripheral input callbacks. Characters
   * received over the relevant peripheral will be handled by
   * slip_input_byte instead
   */
#if UART_CONF_ENABLE
  uart_init(0);
  uart_init(1);
  uart_set_input(SERIAL_LINE_CONF_UART, serial_line_input_byte);
#endif

#if USB_SERIAL_CONF_ENABLE
  usb_serial_init();
  usb_serial_set_input(serial_line_input_byte);
#endif

  serial_line_init();

  INTERRUPTS_ENABLE();

  PUTS(CONTIKI_VERSION_STRING);
  PUTS(BOARD_STRING);

  /* Initialise the H/W RNG engine. */
  random_init(0);

  udma_init();

  process_start(&etimer_process, NULL);
  ctimer_init();

#if PLATFORM_WITH_DUAL_MODE && USB_SERIAL_CONF_ENABLE
  /* wait for USB enumeration to happen for 5 seconds */
  timer_set(&detect_usb_timer, CLOCK_SECOND * 5);

    do {
      uint8_t is_usb_attached(void);

      process_run();

      if(is_usb_attached())
      {
         /* switch to serial radio mode if the usb is attached to host */
         op_mode = DUAL_MODE_OP_MODE_SERIAL_RADIO;
         break;
      }
    } while(!timer_expired(&detect_usb_timer));
#endif

  queuebuf_init();

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

#ifdef SPARROW_DEVICE
  if(SPARROW_DEVICE.init) {
    SPARROW_DEVICE.init();
  }
  PRINTF("Running image: %d\n", SPARROW_DEVICE.get_running_image());
  PRINTF("Reset by %s\n", SPARROW_DEVICE.reset_cause());
#endif /* SPARROW_DEVICE */

#if CRYPTO_CONF_INIT
  crypto_init();
  crypto_disable();
#endif

  netstack_init();
  set_rf_params();

  PRINTF(" Net: ");
  PRINTF("%s\n", NETSTACK_NETWORK.name);
  PRINTF(" MAC: ");
  PRINTF("%s\n", NETSTACK_MAC.name);
  PRINTF(" RDC: ");
  PRINTF("%s\n", NETSTACK_RDC.name);
  PRINTF(" PHY: %s\n",
         &NETSTACK_RADIO == &cc2538_rf_driver ? "CC2538_RF" : "CC1200");
  PRINTF(" PAN-ID: 0x%04x\n", IEEE802154_PANID);
  PRINTF(" RF Channel: %u\n", CC2538_RF_CONF_CHANNEL);
  PRINTF(" Compiled @ %s %s\n", __DATE__, __TIME__);

#if NETSTACK_CONF_WITH_IPV6
  memcpy(&uip_lladdr.addr, &linkaddr_node_addr, sizeof(uip_lladdr.addr));
  process_start(&tcpip_process, NULL);
#if PLATFORM_WITH_DUAL_MODE == 1
  if(op_mode == DUAL_MODE_OP_MODE_STANDARD) {
    /* tcpip_process does not start when in serial-radio mode */
    process_start(&tcpip_process, NULL);
  }
#else /* PLATFORM_WITH_DUAL_MODE == 1 */
  process_start(&tcpip_process, NULL);
#endif /* PLATFORM_WITH_DUAL_MODE == 1 */
#endif /* NETSTACK_CONF_WITH_IPV6 */

  adc_init();

#if PLATFORM_HAS_SENSORS
  process_start(&sensors_process, NULL);
#endif /* PLATFORM_HAS_SENSORS */

#ifdef HAVE_SPARROW_OAM_MFG_INIT
  oam_mfg_init();
#endif

#ifdef HAVE_SPARROW_OAM
  sparrow_oam_init();
#endif /* HAVE_SPARROW_OAM */

#ifdef HAVE_NETSCAN
  netscan_init();
#endif /* HAVE_NETSCAN */

  watchdog_start();

  autostart_start(autostart_processes);

#ifdef MY_APP_INIT_FUNCTION
  MY_APP_INIT_FUNCTION();
#endif /* MY_APP_INIT_FUNCTION */

#ifdef MY_APP_PROCESS
  process_start(&MY_APP_PROCESS, NULL);
#endif /* MY_APP_PROCESS */

  while(1) {
    uint8_t r;
    do {
      /* Reset watchdog and handle polls and events */
      watchdog_periodic();

      r = process_run();
    } while(r > 0);

    /* We have serviced all pending events. Enter a Low-Power mode. */
    lpm_enter();
  }
}
/*---------------------------------------------------------------------------*/

/**
 * @}
 * @}
 */
