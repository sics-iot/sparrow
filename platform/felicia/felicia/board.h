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
 */
#ifndef BOARD_H_
#define BOARD_H_

#include "dev/gpio.h"
#include "dev/nvic.h"

#define PLATFORM_HAS_LEDS    0
#define PLATFORM_HAS_BUTTON  0
#define PLATFORM_HAS_SLIDE_SWITCH 0
#define PLATFORM_HAS_SENSORS 0

#define LEDS_YELLOW          1
#define LEDS_GREEN           2
#define LEDS_RED             0
#define LEDS_BLUE            0
/* Only include the user leds in LEDS_ALL */
#define LEDS_CONF_ALL        0x03
/*---------------------------------------------------------------------------*/
/** \name USB configuration
 *
 * The USB pullup is driven by PC0 (Felicia = TI CC2538EM)
 *
 * @{
 */
#define USB_PULLUP_PORT          GPIO_C_NUM
#define USB_PULLUP_PIN           0 /* PC0 = CC2538EM */
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

/* #define UART0_CTS_PORT           GPIO_D_NUM */
/* #define UART0_CTS_PIN            7 */

/* #define UART0_RTS_PORT           GPIO_B_NUM */
/* #define UART0_RTS_PIN            5 */
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
#define ADC_ALS_PWR_PORT         GPIO_A_NUM /**< ALS power GPIO control port */
#define ADC_ALS_PWR_PIN          7 /**< ALS power GPIO control pin */
#define ADC_ALS_OUT_PIN          6 /**< ALS output ADC input pin on port A */
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
#define SPI0_IN_USE              0

#if SPI0_IN_USE
#define SPI_CLK_PORT             GPIO_B_NUM
#define SPI_CLK_PIN              2
#define SPI_MOSI_PORT            GPIO_B_NUM
#define SPI_MOSI_PIN             4
#define SPI_MISO_PORT            GPIO_B_NUM
#define SPI_MISO_PIN             3
#define SPI_CS_PORT              GPIO_B_NUM
#define SPI_CS_PIN               1
#define SPI_INT_PORT             GPIO_B_NUM
#define SPI_INT_PIN              5          /* Conflict with UART0_RTS_PIN */
#define SPI_SEL_PORT             GPIO_C_NUM /* Currently not used */
#define SPI_SEL_PIN              7
#endif  /* SPI0_IN_USE */

/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Button configuration
 * @{
 */
#define USER_BUTTON_PORT              GPIO_D_NUM
#define USER_BUTTON_PIN               6
#define USER_BUTTON_VECTOR            NVIC_INT_GPIO_PORT_D
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Device string used on startup
 * @{
 */
#define BOARD_STRING "felicia"
/** @} */

#endif /* BOARD_H_ */

/**
 * @}
 * @}
 */
