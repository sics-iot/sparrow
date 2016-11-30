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

#define PLATFORM_HAS_LEDS    1
#define PLATFORM_HAS_BUTTON  1
#define PLATFORM_HAS_SLIDE_SWITCH 1
#define PLATFORM_HAS_SENSORS 1

#define PLATFORM_GET_TEMPERATURE stts751_millikelvin
uint32_t PLATFORM_GET_TEMPERATURE(void);

#define LEDS_YELLOW          1
#define LEDS_GREEN           2
#define LEDS_SYSTEM_YELLOW   4
#define LEDS_SYSTEM_GREEN    8
#define LEDS_RED             0 /* Not in use */
#define LEDS_BLUE            0 /* Not in use */
/* Only include the user leds in LEDS_ALL */
#define LEDS_CONF_ALL        0x03
/*---------------------------------------------------------------------------*/
/** \name USB configuration
 *
 * The USB pullup is driven by PB3
 *
 * @{
 */
#define USB_PULLUP_PORT          GPIO_B_NUM
#define USB_PULLUP_PIN           3 /* For Yanzi IoT-U10 board */
#define USB_PULLUP_ACTIVE_HIGH   0

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
#define ADC_ALS_PWR_PORT         GPIO_A_NUM /**< ALS power GPIO control port */
#define ADC_ALS_PWR_PIN          7 /**< ALS power GPIO control pin */
#define ADC_ALS_OUT_PIN          4 /**< ALS output ADC input pin on port A */
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
#define SPI0_CLK_PORT            GPIO_B_NUM
#define SPI0_CLK_PIN             0
#define SPI0_MOSI_PORT           GPIO_B_NUM
#define SPI0_MOSI_PIN            1
#define SPI0_MISO_PORT           GPIO_B_NUM
#define SPI0_MISO_PIN            2
#endif  /* #if SPI0_IN_USE */

/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Device string used on startup
 * @{
 */
#define BOARD_STRING "IoT-U10"
/** @} */

/**
 * \name Button configuration
 */
#define USER_BUTTON_PORT              GPIO_D_NUM
#define USER_BUTTON_PIN               2
#define USER_BUTTON_VECTOR            NVIC_INT_GPIO_PORT_D

#define SLIDE_SWITCH_PORT             GPIO_B_NUM
#define SLIDE_SWITCH_PIN              4
#define SLIDE_SWITCH_VECTOR           NVIC_INT_GPIO_PORT_B

/**
 * \name I2C pin configuration
 */
#define I2C_SDA_PORT                  GPIO_D_NUM
#define I2C_SDA_PORT_BASE             GPIO_D_BASE
#define I2C_SDA_PIN                   6
#define I2C_SCL_PORT                  GPIO_D_NUM
#define I2C_SCL_PORT_BASE             GPIO_D_BASE
#define I2C_SCL_PIN                   7

#define STTS751_I2C_CONF_ADDRESS        57 /* 0111001(x) x: R/W */

#endif /* BOARD_H_ */

/**
 * @}
 * @}
 */
