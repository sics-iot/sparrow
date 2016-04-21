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


#include "contiki.h"
#include "sparrow-oam.h"
#include "sparrow-uc.h"
#include "sparrow-var.h"

#if !SPARROW_UC_CAN_DO_IPV4 && !SPARROW_UC_CAN_DO_IPV6
#error Sparrow unit controller need at least one of SPARROW_UC_CAN_DO_IPV6 or SPARROW_UC_CAN_DO_IPV4
#endif /* !SPARROW_UC_CAN_DO_IPV4 && !SPARROW_UC_CAN_DO_IPV6 */

static uint64_t last_wd_expire = 0;
static uint32_t uc_wd_last_set = 0;
static uint32_t uc_wd;
static uint32_t uc_time_set = 0;
static uint16_t uc_port;
static uint8_t uc_address[16];
static uint8_t uc_address_type;
static uint8_t uc_change_count = 0;

/*
 * Unit controller implementation.
 *
 * Rules:
 *
 * 1) Writing UC-Address sets mode to "Owned", increments revision
 *    field in UCStatus and watchdog initialized to n seconds. 100 < n
 *    < 1000. This implementation sets 313.
 *
 * 2) UC-Address can only be written when watchdog is zero.
 *
 * 3) When watchdog expires (goes from non-zero to zero), revision
 *    field in UC-Status is incremented and UC-Address, is set to
 *    zero.
 *
 * 4) Watchdog can only be written when it is non-zero.
 *
 * 5) When a value larger than max supported watchdog value is
 *    attempted to be written, watchdog should be set to the maximum
 *    supported value.
 *
 * Format of unit controller address:
 *
 *   3             2 2             1 1
 *   1             4 3             6 5             8 7             0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        type = 1 (IPv6)                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             port                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                               |
 *  |                          IPv6 Address                         |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                      Location identifier                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                     pad with 4 byte zero                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *   3             2               1
 *   1             4               6               8               0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        type = 2 (IPv4)                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             port                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                          IPv4 Address                         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                      Location identifier                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                               |
 *  |                    pad with 16 byte zero                      |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 */

/*---------------------------------------------------------------------------*/
/*
 * Set the unit controller watchdog to the specified time.
 */
uint8_t
sparrow_uc_set_watchdog(uint32_t wd)
{
  if(sparrow_uc_get_watchdog() == 0) {
    return SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED;
  }

  /* Emulate 22 bit counter, so we can have 32 bit milliseconds */
  if(wd > 0x3fffffL) {
    uc_wd = 0xf9fffc18L; /* 0x3fffff seconds in milliseconds */
  } else {
    uc_wd = wd * 1000;
  }
  uc_wd_last_set = uptime_read();
  return SPARROW_TLV_ERROR_NO_ERROR;
}
/*---------------------------------------------------------------------------*/
/*
 * Return watchdog and if "p" is non-NULL, write watchdog value in
 * network byte order to "p".
 */
uint32_t
sparrow_uc_get_watchdog(void)
{
  uint32_t elapsed;
  uint32_t retval;
  if(uc_wd == 0) {
    retval = 0;
  } else {
    elapsed = uptime_elapsed(uc_wd_last_set);
    if(elapsed < uc_wd) {
      retval = (uc_wd - elapsed) / 1000;
    } else {
      retval = 0;
    }

    if(retval == 0) {
      sparrow_uc_set_watchdog_expired();
    }
  }
  return retval;
}
/*---------------------------------------------------------------------------*/
uint64_t
sparrow_uc_get_last_watchdog_expire(void)
{
  return last_wd_expire;
}
/*---------------------------------------------------------------------------*/
/*
 * Set the watchdog to be expired now and notify all instances.
 */
void
sparrow_uc_set_watchdog_expired(void)
{
  if(uc_wd != 0) {
    uc_wd = 0; /* protect from strangeness when time wraps. */
    uc_change_count++;
    last_wd_expire = uptime_read();

    sparrow_oam_notify_all_instances(SPARROW_OAM_NT_UC_WD_EXPIRE, 0);
  }
}
/*---------------------------------------------------------------------------*/
/*
 * Set unit controller address.
 * Assumes "p" points to array of 32 bytes.
 *
 * Returns SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED if watchdog is non-zero or type is unsupported.
 *
 */
uint8_t
sparrow_uc_set_address(const uint8_t *p, uint8_t only_set_address)
{
  uint32_t old_lid;
  uint32_t new_lid;

  if(sparrow_uc_get_watchdog() != 0) {
    return SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED;
  }
  old_lid = sparrow_get_location_id();

  if(p[3] == SPARROW_UC_ADDRESS_TYPES_IPV4) {
    new_lid = sparrow_tlv_get_int32_from_data(&p[12]);
  } else if(p[3] == SPARROW_UC_ADDRESS_TYPES_IPV6) {
    new_lid = sparrow_tlv_get_int32_from_data(&p[24]);
  } else {
    new_lid = 0;
  }

  /*
   * Can not use unit controller address to reset location ID, so all
   * 1:s is equal to zero
   */
  if(new_lid == 0xffffffff) {
    new_lid = 0;
  }

  if(!only_set_address) {
    /*
     * If trying to set a location id, and there is one configured,
     * and they do not match, it is a fail.
     */
    if((new_lid != 0) && (old_lid != 0) && (old_lid != new_lid)) {
      return SPARROW_TLV_ERROR_INVALID_ARGUMENT;
    }
  }

  if(p[3] == SPARROW_UC_ADDRESS_TYPES_IPV4) {
#if !SPARROW_UC_CAN_DO_IPV4
    return SPARROW_TLV_ERROR_INVALID_ARGUMENT;
#endif /* SPARROW_UC_CAN_DO_IPV4 */
    /* sanity checks */
    if((p[8] == 0x00) && (p[9] == 0x00) && (p[10] == 0x00) && (p[11] == 0x00)) {
      /* 0.0.0.0 */
      return SPARROW_TLV_ERROR_INVALID_ARGUMENT;
    }
    if((p[8] == 0xff) && (p[9] == 0xff) && (p[10] == 0xff) && (p[11] == 0xff)) {
      /* 255.255.255.255 */
      return SPARROW_TLV_ERROR_INVALID_ARGUMENT;
    }

    /* Ok, address seem ok, let's do it */
    uc_port = (p[6] << 8) | p[7];
    memcpy(uc_address, &p[8], 4);
    uc_address_type = p[3];
  } else if(p[3] == SPARROW_UC_ADDRESS_TYPES_IPV6) {
#if !SPARROW_UC_CAN_DO_IPV6
    return SPARROW_TLV_ERROR_INVALID_ARGUMENT;
#endif /* SPARROW_UC_CAN_DO_IPV6 */
    uc_port = (p[6] << 8) | p[7];
    memcpy(uc_address, &p[8], 16);
    uc_address_type = p[3];
  } else {
    return SPARROW_TLV_ERROR_INVALID_ARGUMENT;
  }

  uc_time_set = uptime_read() / 1000;

  if(only_set_address) {
    return SPARROW_TLV_ERROR_NO_ERROR;
  }

  sparrow_set_location_id(new_lid);
  uc_change_count++;
  uc_wd = 31000L;
  uc_wd_last_set = uptime_read();
  return SPARROW_TLV_ERROR_NO_ERROR;
}
/*---------------------------------------------------------------------------*/
const uint8_t *
sparrow_uc_get_address(void)
{
  static uint8_t reply[32];
  uint32_t lid;
  uint8_t lid_offset = 0;

  memset(reply, 0, sizeof(reply));

  if(sparrow_uc_get_watchdog() != 0) {
    reply[3] = uc_address_type;
    reply[6] = (uc_port >> 8) & 0xff;
    reply[7] = uc_port & 0xff;

    if(uc_address_type == SPARROW_UC_ADDRESS_TYPES_IPV6) {
      memcpy(&reply[8], uc_address, 16);
      lid_offset = 24;
    } else if(uc_address_type == SPARROW_UC_ADDRESS_TYPES_IPV4) {
      memcpy(&reply[8], uc_address, 4);
      lid_offset = 12;
    }
    if(lid_offset) {
      lid = sparrow_get_location_id();
      reply[lid_offset++] = (lid >> 24) & 0xff;
      reply[lid_offset++] = (lid >> 16) & 0xff;
      reply[lid_offset++] = (lid >> 8) & 0xff;
      reply[lid_offset++] = lid & 0xff;
    }
  }

  return reply;
}
/*---------------------------------------------------------------------------*/
/*
 * Return 32 bit unit controller status.
 */
uint32_t
sparrow_uc_get_status(void)
{
  /* read watchdog to update uc_change_count if needed. */
  sparrow_uc_get_watchdog();
  return uc_change_count;
}
/*---------------------------------------------------------------------------*/
/*
 * Return the boot time in seconds when the unit controller was set.
 */
uint32_t
sparrow_uc_get_time_set(void)
{
  return uc_time_set;
}
/*---------------------------------------------------------------------------*/
/*
 * Return 32 bit communication style.
 */
uint32_t
sparrow_uc_get_communication_style(void)
{
  uint32_t local32;
  local32 = ((uint32_t)(SPARROW_UC_IMPLEMENTED_WATCHDOG_BITS) << 16)
    + ((SPARROW_UC_IMPLEMENTED_CRYPTO_METHODS) << 8)
    + (SPARROW_UC_IMPLEMENTED_ADDRESS_TYPES);
  return local32;
}
/*---------------------------------------------------------------------------*/
/*
 * Compare supplied address to unit controller address and return TRUE
 * if unit controller address is valid and they are the same.
 */
uint8_t
sparrow_uc_is_uc_address(const uint8_t *a, size_t address_len)
{
  /* no address is the same as an expired unit controlelr */
  if(sparrow_uc_get_watchdog() == 0) {
    return FALSE;
  }

  if(uc_address_type == SPARROW_UC_ADDRESS_TYPES_IPV6) {
    if(address_len != 16) {
      return FALSE;
    }
    if(memcmp(a, uc_address, 16) == 0) {
      return TRUE;
    }
  } else if(uc_address_type == SPARROW_UC_ADDRESS_TYPES_IPV4) {
    if(address_len != 4) {
      return FALSE;
    }
    if(memcmp(a, uc_address, 4) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}
/*---------------------------------------------------------------------------*/
