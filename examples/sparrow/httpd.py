#!/usr/bin/python
#
# Copyright (c) 2015-2016, Yanzi Networks AB.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#   1. Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#   2. Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#   3. Neither the name of the copyright holders nor the
#      names of its contributors may be used to endorse or promote products
#      derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

import SimpleHTTPServer, BaseHTTPServer
import socket, urllib2, string

HTTP_PORT = 8000

forward_prefix = None

_httpd = None

class MyRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    # Disable logging DNS lookups
    def address_string(self):
        return str(self.client_address[0])

    def do_GET(self):
        global forward_prefix

        if self.path.startswith("/wget/"):
            addr = self.path[6:]
            i = string.find(addr, "/")
            path = "/"
            if i > 0:
                path = addr[i:]
                addr = addr[0:i]

            if not forward_prefix or not addr.startswith(forward_prefix):
                self.send_error(403)
                return

            url = "http://[" + addr + "]" + path
            print "httpd: forwarding to", url
            data = urllib2.urlopen(url).read()
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(data);
            self.wfile.flush()
            return

        self.path = "/www" + self.path
        print "httpd: Path: ", self.path
        return SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)

def init():
    global _httpd

    _httpd = BaseHTTPServer.HTTPServer(("", HTTP_PORT), MyRequestHandler)
    print "httpd: serving at port", HTTP_PORT

def set_forward_prefix(prefix):
    global forward_prefix
    forward_prefix = prefix
    print "httpd: forward prefix set to \"" + prefix + "\""

def serve_forever():
    global _httpd
    if not _httpd:
        init()
    _httpd.serve_forever()

if __name__ == "__main__":
    # Started standalone
    init()
    serve_forever()
