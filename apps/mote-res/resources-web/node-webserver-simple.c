/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 *
 */

/**
 * \file
 *         A simple web server used to retrieve RPL data
 *         from a mote.
 * \author
 *         Niclas Finne    <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Joel Hoglund    <joel@sics.se>
 */

#include "contiki.h"
#include "httpd-simple.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "print-rpl-info.h"
#include "resource-print-engine.h"

#define SIMPLE_HTML_BODY_LEN 400

PROCESS(node_webserver_simple_process, "Node Simple Web server");

/*
 * Node Simple Webserver process 
 */
PROCESS_THREAD(node_webserver_simple_process, ev, data)
{
  PROCESS_BEGIN();

  httpd_init();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    httpd_appcall(data);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static const char *TOP = "<html><head><title>Node Web Server</title></head><body>\n";
static const char *BOTTOM = "</body></html>\n";
/*---------------------------------------------------------------------------*/
static char buf[SIMPLE_HTML_BODY_LEN];
static int blen;
#define ADD(...) do {                                                    \
    if(sizeof(buf) -1 > blen) {                                          \
      blen += snprintf(&buf[blen], sizeof(buf) - blen - 1, __VA_ARGS__); \
    }                                                                    \
  } while(0)

/*---------------------------------------------------------------------------*/
static
PT_THREAD(send_web_response(struct httpd_state *s))
{
  PSOCK_BEGIN(&s->sout);

  SEND_STRING(&s->sout, TOP);

  memset(buf, 0, SIMPLE_HTML_BODY_LEN);
  blen = 0;

  if((strcmp(s->filename, "/index") == 0) ||
    (strcmp(s->filename, "/index.html") == 0) ||
    (s->filename[1] == '\0')) {
    /* Index or empty page: display the pages to read from the mote */
    ADD("<h1>Mote Info</h1>"
        "<a href=\"");
    ADD("rpl-info\">RPL info</a>"
        "<ol>"
        "<li> <a href=\"");    
    ADD("rpl-info/parent\">parent</a>"
        "</li>"
        "<li> <a href=\"");
    ADD("rpl-info/rank\">rank</a>"
        "</li>"
        "<li> <a href=\"");
    ADD("rpl-info/link-m\">link-metric</a>"
        "</li>"
        "</ol>");
    SEND_STRING(&s->sout, buf);
  } else if(strcmp(s->filename, "/rpl-info/parent") == 0) {
    ADD("<h1>RPL Parent: ");
    blen += print_rpl_parent(&buf[blen], SIMPLE_HTML_BODY_LEN - blen - 1);
    SEND_STRING(&s->sout, buf);
  } else if(strcmp(s->filename, "/rpl-info/rank") == 0) {
    ADD("<h1>RPL Rank: ");
    blen += print_rpl_rank(&buf[blen], SIMPLE_HTML_BODY_LEN - blen - 1);
    SEND_STRING(&s->sout, buf);
  } else if(strcmp(s->filename, "/rpl-info/link-m") == 0) {
    ADD("<h1>Link-metric: ");
    blen += print_rpl_link_metric(&buf[blen], SIMPLE_HTML_BODY_LEN - blen - 1);
    SEND_STRING(&s->sout, buf);
  } else if(strcmp(s->filename, "/rpl-info") == 0) {
    ADD("<h1>RPL Info</h1>"
        "Parent: ");
    blen += print_rpl_parent(&buf[blen], SIMPLE_HTML_BODY_LEN - blen - 1);
    ADD("<br>");
    ADD("Rank: ");
    blen += print_rpl_rank(&buf[blen], SIMPLE_HTML_BODY_LEN - blen - 1);
    ADD("<br>");
    ADD("Link-metric: ");
    blen += print_rpl_link_metric(&buf[blen], SIMPLE_HTML_BODY_LEN - blen - 1);
    SEND_STRING(&s->sout, buf);
  } else {
    ADD("<h1>Page not found</h1>");
    SEND_STRING(&s->sout, buf);
  }
  SEND_STRING(&s->sout, BOTTOM);
  blen = 0;
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
httpd_simple_script_t
httpd_simple_get_script(const char *name)
{
  return send_web_response;
}
/*---------------------------------------------------------------------------*/
