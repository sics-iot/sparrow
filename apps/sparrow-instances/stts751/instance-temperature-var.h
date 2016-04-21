
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


#ifndef INSTANCE_TEMPERATURE_VAR_H_
#define INSTANCE_TEMPERATURE_VAR_H_

#include "sparrow-oam.h"

#define INSTANCE_TEMPERATURE_OBJECT_TYPE      0x0090DA0302010019ULL
#define INSTANCE_TEMPERATURE_LABEL            "Temperature"

/* Variables for instance temperature */
#define VARIABLE_TEMPERATURE             0x100

static const sparrow_oam_variable_t instance_temperature_variables[] = {
{ 0x100,  4, SPARROW_OAM_WRITABILITY_RO, SPARROW_OAM_FORMAT_INTEGER,  0 },
};

#endif /* INSTANCE_TEMPERATURE_VAR_H_ */
