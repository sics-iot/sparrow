/*
 * Copyright (c) 2014-2016, SICS, Swedish ICT AB.
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
 *         Simple logging tool
 * \author
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef YLOG_H_
#define YLOG_H_

#define YLOG_LEVEL_NONE  0
#define YLOG_LEVEL_INFO  1
#define YLOG_LEVEL_DEBUG 3

#ifndef YLOG_LEVEL
#define YLOG_LEVEL YLOG_LEVEL_NONE
#endif

#ifndef YLOG_NAME
#define YLOG_NAME ""
#endif

#if ((YLOG_LEVEL) & YLOG_LEVEL_INFO) == YLOG_LEVEL_INFO
#define YLOG_INFO(...) ylog_info(YLOG_NAME, __VA_ARGS__)
#else
#define YLOG_INFO(...)
#endif /* ((YLOG_LEVEL) & YLOG_LEVEL_INFO) == YLOG_LEVEL_INFO */

#if ((YLOG_LEVEL) & YLOG_LEVEL_DEBUG) == YLOG_LEVEL_DEBUG
#define YLOG_DEBUG(...) ylog_debug(YLOG_NAME, __VA_ARGS__)
#else
#define YLOG_DEBUG(...)
#endif /* ((YLOG_LEVEL) & YLOG_LEVEL_DEBUG) == YLOG_LEVEL_DEBUG */

#define YLOG_ERROR( ...) ylog_error(YLOG_NAME, __VA_ARGS__)

#define YLOG_PRINT( ...) ylog_print(YLOG_NAME, __VA_ARGS__)

#define YLOG_IS_LEVEL(level) (((YLOG_LEVEL) & (level)) == (level))

void ylog_error(const char *name, const char *message, ...);
void ylog_info(const char *name, const char *message, ...);
void ylog_debug(const char *name, const char *message, ...);
void ylog_print(const char *name, const char *message, ...);

#endif /* YLOG_H_ */
