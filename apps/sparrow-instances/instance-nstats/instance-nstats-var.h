
/*-------------------------------------------------------------------*/
/* Sparrow OAM Instance - Do not edit - automatically generated file.*/
/*-------------------------------------------------------------------*/

/*
 * Copyright (c) 2016, SICS, Swedish ICT.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of the copyright holders nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
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

/*-------------------------------------------------------------------*/
/* Sparrow OAM Instance - Do not edit - automatically generated file.*/
/*-------------------------------------------------------------------*/


#ifndef INSTANCE_NSTATS_VAR_H_
#define INSTANCE_NSTATS_VAR_H_

#include "sparrow-oam.h"

#define INSTANCE_NSTATS_OBJECT_TYPE      0x0090DA0303010023ULL
#define INSTANCE_NSTATS_LABEL            "Network Statistics"

/* Enum types for instance nstats */

typedef enum {
  NSTATS_PROBE_NBR_PROBE_SINGLE          = 0,
  NSTATS_PROBE_NBR_PROBE_UNTIL_VALID     = 1,
  NSTATS_PROBE_NBR_PROBE_AND_SWITCH      = 3,
} nstats_probe_nbr_t;

typedef enum {
  NSTATS_DATA_TYPE_NONE                  = 0,
  NSTATS_DATA_TYPE_DEFAULT_INFO          = 1,
  NSTATS_DATA_TYPE_PARENT_INFO           = 2,
  NSTATS_DATA_TYPE_BEACONS               = 3,
  NSTATS_DATA_TYPE_NETSELECT             = 4,
  NSTATS_DATA_TYPE_RADIO                 = 5,
  NSTATS_DATA_TYPE_NET_CONFIG            = 6,
  NSTATS_DATA_TYPE_SLEEP                 = 7,
} nstats_data_type_t;

/* Variables for instance nstats */
#define VARIABLE_NSTATS_VERSION          0x100
#define VARIABLE_NSTATS_CAPABILITIES     0x101
#define VARIABLE_NSTATS_PUSH_PERIOD      0x102
#define VARIABLE_NSTATS_PUSH_TIME        0x103
#define VARIABLE_NSTATS_PUSH_PORT        0x104
#define VARIABLE_NSTATS_RECOMMENDED_PARENT 0x105
#define VARIABLE_NSTATS_DATA             0x106
#define VARIABLE_NSTATS_PREFERRED_PARENT 0x107
#define VARIABLE_NSTATS_PROBE_NEIGHBOR   0x108

static const sparrow_oam_variable_t instance_nstats_variables[] = {
{ 0x100,  4, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 },
{ 0x101,  8, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 },
{ 0x102,  4, SPARROW_OAM_WRITABILITY_RW, SPARROW_OAM_FORMAT_INTEGER,  0 },
{ 0x103,  4, SPARROW_OAM_WRITABILITY_RW, SPARROW_OAM_FORMAT_INTEGER,  0 },
{ 0x104,  4, SPARROW_OAM_WRITABILITY_RW, SPARROW_OAM_FORMAT_INTEGER,  0 },
{ 0x105, 16, SPARROW_OAM_WRITABILITY_RW, SPARROW_OAM_FORMAT_INTEGER,  0 },
{ 0x106,  4, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_ARRAY,    SPARROW_OAM_VECTOR_DEPTH_DONT_CHECK },
{ 0x107, 32, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 },
{ 0x108, 16, SPARROW_OAM_WRITABILITY_RW, SPARROW_OAM_FORMAT_INTEGER,  0 },
};

#endif /* INSTANCE_NSTATS_VAR_H_ */
