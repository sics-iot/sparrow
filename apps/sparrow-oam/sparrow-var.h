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
 *         Functions for handling common variables in Sparrow instances.
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef SPARROW_VAR_H_
#define SPARROW_VAR_H_

#include "sparrow-tlv.h"
#include "sparrow-oam.h"

/* The mandatory discovery variables */
#define VARIABLE_OBJECT_TYPE                0x000
#define VARIABLE_OBJECT_ID                  0x001
#define VARIABLE_OBJECT_LABEL               0x002
#define VARIABLE_NUMBER_OF_INSTANCES        0x003
#define VARIABLE_OBJECT_SUB_TYPE            0x004
#define VARIABLE_EVENT_ARRAY                0x005

/* Instance 0 variables used by multiple files */
#define VARIABLE_UNIT_CONTROLLER_WATCHDOG   0x0c0
#define VARIABLE_UNIT_CONTROLLER_STATUS     0x0c1
#define VARIABLE_UNIT_CONTROLLER_ADDRESS    0x0c2
#define VARIABLE_COMMUNICATION_STYLE        0x0c3
#define VARIABLE_CONFIGURATION_STATUS       0x0c8
#define VARIABLE_UNIT_BOOT_TIMER            0x0c9
#define VARIABLE_HARDWARE_RESET             0x0ca
#define VARIABLE_READ_THIS_COUNTER          0x0cb
#define VARIABLE_SW_REVISION                0x0cc
#define VARIABLE_TLV_GROUPING               0x0cd
#define VARIABLE_LOCATION_ID                0x0ce
#define VARIABLE_OWNER_ID                   0x0cf
#define VARIABLE_SLEEP_DEFAULT_AWAKE_TIME   0x0d0
#define VARIABLE_CHASSIS_CAPABILITIES       0x0e0
#define VARIABLE_CHASSIS_BOOTLOADER_VERSION 0x0e1
#define VARIABLE_CHASSIS_FACTORY_DEFAULT    0x0e2
#define VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX 0x0e3
#define VARIABLE_IDENTIFY_CHASSIS           0x0e4
#define VARIABLE_RESET_CAUSE                0x0e5
#define VARIABLE_RECEIVED_OAM_PDUS          0x0e6
#define VARIABLE_TRANSMITTED_OAM_PDUS       0x0e7
#define VARIABLE_TRANSMITTED_EVENT_OAM_PDUS 0x0e8
#define VARIABLE_CHASSIS_INFO_HIGHLIGHTS    0x0e9
#define VARIABLE_CHASSIS_ACTIVITY_LEVEL     0x0ea
#define VARIABLE_CHASSIS_RESTART_NETSELECT  0x0eb
#define VARIABLE_CHASSIS_ACTIVITY_CYCLES    0x0ec
#define VARIABLE_CHASSIS_ACTIVITY_COMMAND   0x0ed
#define VARIABLE_CHASSIS_WAKEUPP_INFO       0x0ee
#define VARIABLE_PSP_PORTAL_STATUS          0x10d

sparrow_tlv_error_t sparrow_var_check_tlv(const sparrow_tlv_t *t, uint8_t isPSP);

void sparrow_var_update_event_arrays(void);

#endif /* SPARROW_VAR_H_ */
