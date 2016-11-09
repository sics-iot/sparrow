#!/usr/bin/env python
#
# Copyright (c) 2013-2016, Yanzi Networks
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
# Author: Joakim Eriksson, joakime@sics.se
#
# SerialRadio module
# library for setting up connections to a Sparrow serial radio.
#

import tlvlib, socket, thread, time, binascii

SLIP_END = chr(0300)
SLIP_ESC = chr(0333)
SLIP_ESC_END = chr(0334)
SLIP_ESC_ESC = chr(0335)

class Slip:

    slip_packets = []
    slip_buf = ""
    slip_mode = None

    def encode(self, data):
        output = ""
        for d in data:
            if d == SLIP_END:
                output += SLIP_ESC + SLIP_ESC_END
            elif d == SLIP_ESC:
                output += SLIP_ESC + SLIP_ESC_ESC
            else:
                output += d
        output += SLIP_END
        return output

    # decode SLIP
    def decode(self, new_data):
        for d in new_data:
            if self.slip_mode == None:
                if d == SLIP_END:
                    if self.slip_buf != "":
                        self.slip_packets = self.slip_packets + [self.slip_buf]
                        self.slip_buf = ""
                elif d == SLIP_ESC:
                    self.slip_mode = SLIP_ESC
                else:
                    self.slip_buf += d
            else: # Escape mode
                if d == SLIP_ESC_END:
                    self.slip_buf += SLIP_END
                elif d == SLIP_ESC_ESC:
                    self.slip_buf += SLIP_ESC
                else:
                    print "SLIP Error"
                self.slip_mode = None
        if self.slip_packets != []:
            rval = self.slip_packets
            self.slip_packets = []
            return rval


class SerialRadioConnection:

    DEBUG = False
    packets = []
    socket = None
    thread = None
    slip = None

    def connect(self, host = "localhost", port = 9999):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((host, port))
        self.slip = Slip()
        self.thread = thread.start_new_thread(self.reader, ())

    def set_debug(self, d):
        print "Debug set to:", d
        self.DEBUG = d

    def reader(self):
        while(self.socket != None):
            self.read()

    def read(self):
        data = self.socket.recv(400)
        if len(data) > 0:
            packets = self.slip.decode(data)
            if packets != None:
                for p in packets:
                    if p != "":
                        if p[0] == '\r':
                            if self.DEBUG:
                                print "DEBUG:", p[1:]
                        else:
                            if self.DEBUG:
                                print "RECV:", binascii.hexlify(p)
                            self.packets = self.packets + [p]
                            #print binascii.hexlify(p)

    # return the next encap data received
    def get_next(self):
        if self.packets != []:
            p = self.packets[0]
            self.packets = self.packets[1:]
            enc = tlvlib.EncapHeader()
            enc.unpack(p)
            return enc
        return None

    def get_next_block(self, timeout = 5):
        start = time.time()
        while (time.time() - start < timeout):
            if self.has_next():
                return self.get_next()
            time.sleep(0.1) # short sleep to not busy loop...
        return None

    def has_next(self):
        return self.packets != []

    def send(self, data):
        enc_pdu = tlvlib.EncapHeader()
        enc_pdu.set_serial(data)
        serial_pdu = self.slip.encode(enc_pdu.pack())
        if self.DEBUG:
            print "SEND:", binascii.hexlify(serial_pdu)
        self.socket.send(serial_pdu)

    def close(self):
        self.socket.close()
        self.socket = None
