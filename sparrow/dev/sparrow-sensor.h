/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         Sparrow Sensor API
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef SPARROW_SENSOR_H_
#define SPARROW_SENSOR_H_

#include <stdint.h>

#define SPARROW_SENSOR_STATUS_OK_TOO_HIGH      2
#define SPARROW_SENSOR_STATUS_OK_TOO_LOW       1
#define SPARROW_SENSOR_STATUS_OK               0
#define SPARROW_SENSOR_STATUS_TIMEOUT         -1
#define SPARROW_SENSOR_STATUS_ERROR_HARDWARE  -2
#define SPARROW_SENSOR_STATUS_ERROR_INTERNAL  -3
#define SPARROW_SENSOR_STATUS_ERROR_ARGS      -4

#define SPARROW_SENSOR_IS_STATUS_OK(status)    \
  ((status) >= SPARROW_SENSOR_STATUS_OK)

typedef int      sparrow_sensor_status_t;
typedef uint16_t sparrow_sensor_type_t;

typedef struct sparrow_sensor sparrow_sensor_t;
typedef void (* sparrow_sensor_callback_t)(const sparrow_sensor_t *sensor,
                                           sparrow_sensor_status_t status,
                                           int32_t sensor_value);
struct sparrow_sensor {

  const char *name;
  sparrow_sensor_type_t type;

  void (* request_reading)(sparrow_sensor_callback_t callback);
  sparrow_sensor_status_t (* read_value)(int32_t *value);

};

#endif /* SPARROW_SENSOR_H_ */
