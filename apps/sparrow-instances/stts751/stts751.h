/*
 * Copyright (c) 2013-2016, Yanzi Networks AB.
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
 */

/**
 * \file
 *         STTS751 Temperature sensor chip driver
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 */

#ifndef STTS751_H_
#define STTS751_H_

#include <stdint.h>

/*
 * Registers
 */
#define STTS751_TEMPERATURE_VALUE_HIGH_BYTE      0x00
#define STTS751_STATUS_REG                       0x01
#define STTS751_TEMPERATURE_VALUE_LOW_BYTE       0x02
#define STTS751_CONFIGURATION                    0x03
#define STTS751_CONVERSION_RATE                  0x04
#define STTS751_TEMPERATURE_HIGH_LIMIT_HIGH_BYTE 0x05
#define STTS751_TEMPERATURE_HIGH_LIMIT_LOW_BYTE  0x06
#define STTS751_TEMPERATURE_LOW_LIMIT_HIGH_BYTE  0x07
#define STTS751_TEMPERATURE_LOW_LIMIT_LOW_BYTE   0x08
#define STTS751_ONE_SHOT                         0x0f
#define STTS751_THERM_LIMIT                      0x20
#define STTS751_THERM_HYSTERESIS                 0x21
#define STTS751_SMBUS_TIMEOUT_ENABLE             0x22
#define STTS751_PRODUCT_ID_REGISTER              0xfd
#define STTS751_MANUFACTURER_ID                  0xfe
#define STTS751_REVISION_NUMBER                  0xff

#define STTS751_STATUS_REG_BUSY                  0x80

#define STTS751_STATUS_OK      0
#define STTS751_STATUS_TIMEOUT 1
#define STTS751_STATUS_EINVAL  2
#define STTS751_MILLIKELVIN_ERRORVAL 0xffffffff

uint32_t stts751_millikelvin(void);

#endif /* STTS751_H_ */
