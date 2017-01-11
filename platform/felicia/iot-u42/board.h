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
/** \addtogroup cc2538
 * @{
 *
 * \defgroup felicia felicia module peripherals
 *
 * Defines related to the felicia module
 *
 * @{
 *
 * \file
 * Header file with definitions related to the I/O connections on the felicia module
 *
 * \note   Do not include this file directly. It gets included by contiki-conf
 *         after all relevant directives have been set.
 *
 * IoT-u42 has two radios and therefore has support for running DUAL radio.
 */
#ifndef BOARD_H_
#define BOARD_H_

#include "dev/gpio.h"
#include "dev/nvic.h"

#define PLATFORM_HAS_LEDS    1
#define PLATFORM_HAS_BUTTON  0
#define PLATFORM_HAS_SLIDE_SWITCH 0
#define PLATFORM_HAS_SENSORS 0

/* #define PLATFORM_GET_TEMPERATURE stts751_millikelvin */
/* uint32_t PLATFORM_GET_TEMPERATURE(void); */

#define LEDS_YELLOW          1
#define LEDS_GREEN           2
#define LEDS_SYSTEM_YELLOW   1
#define LEDS_SYSTEM_GREEN    2
#define LEDS_RED             0
#define LEDS_BLUE            0
/* Only include the user leds in LEDS_ALL */
#define LEDS_CONF_ALL        0x03
/*---------------------------------------------------------------------------*/
/** \name Radio configuration
 *
 * @{
 */

#ifndef CC2538_RF_CONF_TX_POWER
/* Set default TX power to 11 dBm on IoT-U10+. */
#define CC2538_RF_CONF_TX_POWER  0x62
#endif /* CC2538_RF_CONF_TX_POWER */

#define CC2538_CONF_CC2592       1
#define CC2538_HGM_PORT          GPIO_D_NUM
#define CC2538_HGM_PIN           2

/** @} */
/*---------------------------------------------------------------------------*/
/** \name USB configuration
 *
 * The USB pullup is driven by PC6
 *
 * @{
 */
#define USB_PULLUP_PORT          GPIO_C_NUM
#define USB_PULLUP_PIN           1 /* For Yanzi IoT-U42 board */
#define USB_PULLUP_ACTIVE_HIGH   1

/** @} */
/*---------------------------------------------------------------------------*/
/** \name UART configuration
 *
 * On the felicia module, the UART (XDS back channel) is connected to the
 * following ports/pins
 * - RX:  PA0
 * - TX:  PA1
 * - CTS: PB0 //mg: verify that
 * - RTS: PD3 //mg: verify that
 *
 * @{
 */
#define UART0_RX_PORT            GPIO_A_NUM
#define UART0_RX_PIN             0

#define UART0_TX_PORT            GPIO_A_NUM
#define UART0_TX_PIN             1

/* #define UART0_CTS_PORT           GPIO_B_NUM */
/* #define UART0_CTS_PIN            0 */

/* #define UART0_RTS_PORT           GPIO_D_NUM */
/* #define UART0_RTS_PIN            3 */
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name ADC configuration
 *
 * These values configure which CC2538 pins and ADC channels to use for the ADC
 * inputs.
 *
 * ADC inputs can only be on port A.
 * @{
 */
/* #define ADC_ALS_PWR_PORT         GPIO_A_NUM /\**< ALS power GPIO control port *\/ */
/* #define ADC_ALS_PWR_PIN          7 /\**< ALS power GPIO control pin *\/ */
/* #define ADC_ALS_OUT_PIN          6 /\**< ALS output ADC input pin on port A *\/ */
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name SPI configuration
 *
 * These values configure which CC2538 pins to use for the SPI lines. Both
 * SPI instances can be used independently by providing the corresponding
 * port / pin macros.
 * @{
 */
/* #define SPI0_IN_USE              0 */

/* #if SPI0_IN_USE */
/* #define SPI0_CLK_PORT            GPIO_B_NUM */
/* #define SPI0_CLK_PIN             2 */
/* #define SPI0_MOSI_PORT           GPIO_B_NUM */
/* #define SPI0_MOSI_PIN            4 */
/* #define SPI0_MISO_PORT           GPIO_B_NUM */
/* #define SPI0_MISO_PIN            5 */
/* #endif  /\* #if SPI0_IN_USE *\/ */

/* Configuration of CC1200 for the Gordon                                  */
/**
 * \name SPI (SSI0) configuration
 *
 * These values configure which CC2538 pins to use for the SPI (SSI0) lines,
 * reserved exclusively for the CC1200 RF transceiver.  These pins are not
 * exposed to any connector, and should be avoid to use it.
 * TX -> MOSI, RX -> MISO
 * @{
 */
#define SPI0_CLK_PORT             GPIO_D_NUM
#define SPI0_CLK_PIN              3
#define SPI0_TX_PORT              GPIO_D_NUM
#define SPI0_TX_PIN               1
#define SPI0_RX_PORT              GPIO_A_NUM
#define SPI0_RX_PIN               7

/**
 * \name CC1200 configuration
 *
 * These values configure the required pins to drive the CC1200
 * None of the following pins are exposed to any connector, kept for internal
 * use only
 * @{
 */
#define CC1200_SPI_INSTANCE         0
#define CC1200_SPI_SCLK_PORT        SPI0_CLK_PORT
#define CC1200_SPI_SCLK_PIN         SPI0_CLK_PIN
#define CC1200_SPI_MOSI_PORT        SPI0_TX_PORT
#define CC1200_SPI_MOSI_PIN         SPI0_TX_PIN
#define CC1200_SPI_MISO_PORT        SPI0_RX_PORT
#define CC1200_SPI_MISO_PIN         SPI0_RX_PIN
#define CC1200_SPI_CSN_PORT         GPIO_A_NUM
#define CC1200_SPI_CSN_PIN          4
#define CC1200_GDO0_PORT            GPIO_B_NUM
#define CC1200_GDO0_PIN             4
#define CC1200_GDO2_PORT            GPIO_D_NUM
#define CC1200_GDO2_PIN             0
#define CC1200_RESET_PORT           GPIO_A_NUM
#define CC1200_RESET_PIN            6
/* What GPIO from the radio will trigger IRQ - only GDO0 is needed */
#define CC1200_GPIOx_VECTOR         GPIO_B_IRQn

#define CC1200_CONF_AUTOCAL         1

/* only works with devices with two radios - test with two radios */
#define DUAL_RADIO_CONF_RADIO_1 cc2538_rf_driver
#define DUAL_RADIO_CONF_RADIO_2 cc1200_driver

/* Support dual radio, 2.4GHz radio (default), or 868MHz */
#if WITH_DUAL_RADIO
#define NETSTACK_CONF_RADIO         dual_radio_driver
#define NULLRDC_CONF_ACK_WAIT_TIME  (RTIMER_SECOND / 100)
#elif WITH_868
#define NETSTACK_CONF_RADIO         cc1200_driver
#define NULLRDC_CONF_ACK_WAIT_TIME  (RTIMER_SECOND / 100)
#else /* WITH_868 */
#define NETSTACK_CONF_RADIO         cc2538_rf_driver
#endif /* WITH_868 */

#define CC1200_CONF_USE_GPIO2       0
#define CC1200_CONF_USE_RX_WATCHDOG 1

/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Device string used on startup
 * @{
 */
#define BOARD_STRING "IoT-U42"
/** @} */

/**
 * \name I2C pin configuration
 */
#define I2C_SDA_PORT                  GPIO_D_NUM
#define I2C_SDA_PORT_BASE             GPIO_D_BASE
#define I2C_SDA_PIN                   7
#define I2C_SCL_PORT                  GPIO_D_NUM
#define I2C_SCL_PORT_BASE             GPIO_D_BASE
#define I2C_SCL_PIN                   6

#endif /* BOARD_H_ */

/**
 * @}
 * @}
 */
