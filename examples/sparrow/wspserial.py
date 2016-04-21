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
#

import wsplugin, serialradio, sys, thread, json

# The plugin class
class SerialCommands(wsplugin.DemoPlugin):

    def get_commands(self):
        return ["sniff", "rssi"]

    def handle_command(self, wsdemo, cmds):
        if cmds[0] == "rssi":
            thread.start_new_thread(rssi_scan, (wsdemo,))
            return True
        elif cmds[0] == "sniff":
            thread.start_new_thread(sniff, (wsdemo,))
            return True
        return False

def hexdump(src, length=16):
    FILTER = ''.join([(len(repr(chr(x))) == 3) and chr(x) or '.' for x in range(256)])
    lines = []
    for c in xrange(0, len(src), length):
        chars = src[c:c+length]
        hex = ' '.join(["%02x" % ord(x) for x in chars])
        printable = ''.join(["%s" % ((ord(x) <= 127 and FILTER[ord(x)]) or '.') for x in chars])
        lines.append("%04x  %-*s  %s\n" % (c, length*3, hex, printable))
    return ''.join(lines)


con = None

def serial_connect(ws):
    global con
    if con != None:
        print "Already Connected to TCP socket"
        return con
    print "Connecting to TCP socket of NBR"
    con = serialradio.SerialRadioConnection()
    con.connect()
#    con.set_debug(True)
    return con

def serial_disconnect(ws):
    global con
    if con != None:
        print "disconnection from serial radio"
    con.send("!m\001")
    con.close()
    con = None

def sniff(ws):
    ws.stop = False
    con = serial_connect(ws)
    print "Configuring radio for sniffing. ", con
    con.send("!m\002")
    while not ws.stop:
        try:
            data = con.get_next_block(1)
            sniff_data = []
            if data != None:
                # skip the first 3 chars and dump the rest to javascript...
                for d in data.serial_data[2:]:
                    sniff_data = sniff_data + [ord(d)]
                ws.sendMessage(json.dumps({'sniff':sniff_data, 'hex':hexdump(data.serial_data[2:])}))
        except Exception as e:
            print "Sniff - Unexpected error:", sys.exc_info()[0], "E:", e
            break
    serial_disconnect(ws)

def rssi_scan(ws):
    ws.stop = False
    con = serial_connect(ws)
    print "Configuring radio for RSSI scan.", con
    con.send("!c\001")
    con.send("!m\003")
    while not ws.stop:
        try:
            data = con.get_next_block(1)
            #print "Received ", i, " => ", data
            rssi = []
            if data != None:
                # skip the first 3 chars and dump the rest to javascript...
                for d in data.serial_data[3:]:
                    v = ord(d)
                    if v >= 128: # negative...
                        v = v - 256
                    rssi = rssi + [v]
                ws.sendMessage(json.dumps({'rssi':rssi}))
        except Exception as e:
            print "RSSI - Unexpected error:", sys.exc_info()[0], e
            break
    serial_disconnect(ws)
