/*
 * Copyright (c) 2012-2016, Yanzi Networks AB.
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
 *         Unitcontroller helper functions
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef SPARROW_UC_H_
#define SPARROW_UC_H_

#include "contiki-conf.h"

#define SPARROW_UC_IMPLEMENTED_WATCHDOG_BITS 22
#define SPARROW_UC_ADDRESS_TYPES_IPV4 0x01
#define SPARROW_UC_ADDRESS_TYPES_IPV6 0x02
/* This implementation handles IPv4/UDP and IPv6/UDP */
#define SPARROW_UC_IMPLEMENTED_ADDRESS_TYPES (SPARROW_UC_ADDRESS_TYPES_IPV4 | SPARROW_UC_ADDRESS_TYPES_IPV6)
#define SPARROW_UC_IMPLEMENTED_CRYPTO_METHODS 0x00 /* no crypto yet */

#ifndef SPARROW_UC_CAN_DO_IPV4
#define SPARROW_UC_CAN_DO_IPV4 0
#endif /* SPARROW_UC_CAN_DO_IPV4 */

#ifndef SPARROW_UC_CAN_DO_IPV6
#define SPARROW_UC_CAN_DO_IPV6 1
#endif /* SPARROW_UC_CAN_DO_IPV6 */

uint32_t sparrow_uc_get_watchdog(void);
uint8_t sparrow_uc_set_watchdog(uint32_t wd);
void sparrow_uc_set_watchdog_expired(void);
uint64_t sparrow_uc_get_last_watchdog_expire(void);

uint32_t sparrow_uc_get_status(void);

uint32_t sparrow_uc_get_time_set(void);

uint8_t sparrow_uc_set_address(const uint8_t *p, uint8_t only_set_address);
const uint8_t *sparrow_uc_get_address(void);

uint8_t sparrow_uc_is_uc_address(const uint8_t *a, size_t address_len);

uint32_t sparrow_uc_get_communication_style(void);

#endif /* SPARROW_UC_H_ */
