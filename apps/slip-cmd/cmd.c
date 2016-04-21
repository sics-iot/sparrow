/*
 * Copyright (c) 2011-2016, Swedish Institute of Computer Science.
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
 *         Simple command handler
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "cmd.h"
#include <string.h>

#ifndef CMD_CONF_OUTPUT
#define CMD_OUTPUT slip_cmd_input
#else
#define CMD_OUTPUT CMD_CONF_OUTPUT
#endif /* CMD_CONF_OUTPUT */

#ifdef CMD_CONF_ERROR
#define CMD_ERROR CMD_CONF_ERROR
void CMD_ERROR(cmd_error_t error, const uint8_t *data, int data_len);
#endif /* CMD_CONF_ERROR */

void CMD_OUTPUT(const uint8_t *data, int data_len);

extern const cmd_handler_t cmd_handlers[];

/*---------------------------------------------------------------------------*/
static int
run_cmd(const uint8_t *data, int data_len)
{
  int i;
  for(i = 0; cmd_handlers[i] != NULL; i++) {
    if(cmd_handlers[i](data, data_len)) {
      /* Command has been handled */
      return 1;
    }
  }

  if(data_len > 0) {
    /* Unknown command */
#ifdef CMD_ERROR
    CMD_ERROR(CMD_ERROR_UNKNOWN_COMMAND, data, data_len);
#endif /* CMD_ERROR */
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
void
cmd_input(const uint8_t *data, int data_len)
{
  int i;
  uint8_t size;
  if(data_len > 1 && *data == CMD_TYPE_GROUP) {
    /* Command group */
    for(i = 1; i < data_len;) {
      size = data[i];
      if(i + size + 1 <= data_len) {
        run_cmd(data + i + 1, size);
        i += size + 1;
      } else {
        /* Truncated command */
#ifdef CMD_ERROR
        CMD_ERROR(CMD_ERROR_TRUNCATED_COMMAND, &data[i + 1], data_len - i - 1);
#endif /* CMD_ERROR */
        break;
      }
    }
  } else {
    run_cmd(data, data_len);
  }
}
/*---------------------------------------------------------------------------*/
void
cmd_send(const uint8_t *data, int data_len)
{
  CMD_OUTPUT(data, data_len);
}
/*---------------------------------------------------------------------------*/
int
cmd_group_start(uint8_t *buf, int buf_size)
{
  if(buf_size > 1) {
    buf[0] = CMD_TYPE_GROUP;
    return 1;
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int
cmd_group_add(uint8_t *buf, int offset, int buf_size,
              const uint8_t *cmd, int cmd_len)
{
  if(offset < 0) {
    return offset;
  }
  if(offset + cmd_len + 1 <= buf_size) {
    buf[offset] = cmd_len;
    memcpy(&buf[offset + 1], cmd, cmd_len);
    return offset + cmd_len + 1;
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int
cmd_group_end(uint8_t *buf, int offset, int buf_size)
{
  return offset;
}
/*---------------------------------------------------------------------------*/
