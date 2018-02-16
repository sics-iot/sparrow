/*
 * Copyright (c) 2017, RISE SICS AB.
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
 *         Sparrow TLV variables for instance neighbours
 * \author
 *         Bengt Ahlgren <bengta@sics.se>
 */

/*
 * Modelled after instance-rtable* by
 * Fredrik Ljungberg <flag@yanzinetworks.com>
 * Joakim Eriksson <joakime@sics.se>
 * Niclas Finne <nfi@sics.se>
 */

#ifndef _INSTANCE_NBR_VAR_H_
#define _INSTANCE_NBR_VAR_H_

#include "sparrow-oam.h"

/* Object 0x70B3D57D52020001 "Neighbours" */
#define INSTANCE_NBR_OBJECT_TYPE    0x70B3D57D52020001ULL
#define INSTANCE_NBR_LABEL          "Neighbours"

#define VARIABLE_NBR_COUNT     0x100 /* Count of neighbours */
#define VARIABLE_NBR_TABLE     0x101 /* Neighbour table */
#define VARIABLE_NBR_REVISION  0x102 /* Neighbour table revision - write=update */

/*
 * Client "workflow":
 * 1. Set VARIABLE_NBR_REVISION (value ignored): instance updates local neighbour table
 * 2. Get VARIABLE_NBR_REVISION
 * 3. Get VARIABLE_NBR_COUNT
 * 4. Get VARIABLE_NBR_TABLE
 * 5. Get VARIABLE_NBR_REVISION - if changed from step 2/5, retry from step 3
 */

static const sparrow_oam_variable_t instance_nbr_variables[] = {
    { VARIABLE_NBR_COUNT,    4, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER, 0 },
    { VARIABLE_NBR_TABLE,   64, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_ARRAY,   SPARROW_OAM_VECTOR_DEPTH_DONT_CHECK },
    { VARIABLE_NBR_REVISION, 4, SPARROW_OAM_WRITABILITY_RW, SPARROW_OAM_FORMAT_INTEGER, 0 },
};

#endif /* _INSTANCE_NBR_VAR_H_ */
