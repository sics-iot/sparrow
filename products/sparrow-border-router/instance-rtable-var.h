/*
 * Copyright (c) 2016, Yanzi Networks AB.
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
 *         Sparrow TLV variables for instance routing table
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef INSTANCE_RTABLE_VAR_H_
#define INSTANCE_RTABLE_VAR_H_

#include "sparrow-oam.h"

/* Object 0x0090DA0302010015 "Routing table" */
#define INSTANCE_RTABLE_OBJECT_TYPE    0x0090DA0302010015ULL
#define INSTANCE_RTABLE_LABEL          "Routing table"

#define VARIABLE_TABLE_LENGTH 0x100
#define VARIABLE_TABLE_REVISION 0x101
#define VARIABLE_TABLE 0x102
#define VARIABLE_NETWORK_ADDRESS 0x103

static const sparrow_oam_variable_t instance_rtable_variables[] = {
  { 0x100,  4, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 }, /* VARIABLE_TABLE_LENGTH */
  { 0x101,  4, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 }, /* VARIABLE_TABLE_REVISION */
  { 0x102, 64, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_ARRAY  , SPARROW_OAM_VECTOR_DEPTH_DONT_CHECK }, /* VARIABLE_TABLE */
  { 0x103, 16, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 }, /* VARIABLE_NETWORK_ADDRESS */
};

#endif /* INSTANCE_RTABLE_VAR_H_ */
