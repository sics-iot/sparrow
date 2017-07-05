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
 *
 */

/**
 * \file
 *         Uptime API - time since last boot
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 */

/**
 * \addtogroup sys
 * @{
 */

/**
 * \defgroup uptime Contiki Uptime library
 *
 * The uptime library provides functions for getting the time since
 * boot in different formats. The uptime resolution is milliseconds
 * but it is up to the platform how often it is updated. The uptime
 * will never wrap or decrease.
 *
 * @{
 */

#ifndef UPTIME_H_
#define UPTIME_H_

#include <stdint.h>

/*
 * milliseconds since boot
 */
uint64_t uptime_read(void);

/*
 * seconds since boot
 */
uint32_t uptime_seconds(void);

/*
 * IEEE 64 bit time format.
 * 32 bit seconds + 1 bit sign + 31 bits * nanoseconds.
 */
uint64_t uptime_ieee64(void);

/*
 * IEEE 64 bit time format.
 * 32 bit seconds only
 */
#define uptime_ieee64in32() uptime_seconds()

/*
 * Give number of milliseconds that have elapsed since "when".
 */
uint64_t uptime_elapsed(uint64_t when);

/*
 * Give number of milliseconds until "future".
 */
int32_t uptime_milliseconds_until(uint64_t future);

#endif /* UPTIME_H_ */

/** @} */
/** @} */
