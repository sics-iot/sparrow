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
 *         Sparrow beacon format implemention
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef SPARROW_BEACON_H_
#define SPARROW_BEACON_H_

#include <stdint.h>

uint8_t sparrow_beacon_get_value_from_beacon(const uint8_t *beacon,
                                             uint8_t beacon_len,
                                             const uint8_t *type,
                                             uint8_t type_len,
                                             uint8_t *dst,
                                             uint8_t dst_len);

uint8_t sparrow_beacon_get_location(const uint8_t *bp, uint8_t len,
                                    uint8_t *dst, uint8_t dst_len);

uint8_t sparrow_beacon_get_etx(const uint8_t *bp, uint8_t len,
                               uint8_t *dst, uint8_t dst_len);

uint8_t sparrow_beacon_get_address(const uint8_t *bp, uint8_t len,
                                   uint8_t *dst, uint8_t dst_len);

uint8_t sparrow_beacon_get_owner(const uint8_t *bp, uint8_t len,
                                 uint8_t *dst, uint8_t dst_len);

int sparrow_beacon_calculate_payload_length(const uint8_t *buf,
                                            uint8_t max_len);

#endif /* SPARROW_BEACON_H_ */
