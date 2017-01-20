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
#         Niclas Finne, nfi@sics.se
#
# Packet sniffer for Sparrow Serial Radio and Sparrow Border Router
# that produce a PCAP file or outputs PCAP to standard out.
# Tools like Wireshark can be used to analyze PCAP data.
#

import sys, getopt
import serialradio
import binascii, struct, time, array

_ETH_DATA = array.array('B', [0xaf, 0xab, 0xac, 0xad, 0xae, 0xaf, 0x42, 0xfb, 0x9f, 0x81, 0x5a,
            0x81, 0x80, 0x9a]).tostring()

def init_pcap(out):
    # pcap file header */
    PCAPHDR = struct.pack("!LHHLLLL", 0xa1b2c3d4, 0x0002, 0x004, 0, 0, 4096, 1)
    out.write(PCAPHDR)

def export_packet_data(out, data):
    # pcap packet header with timestamp
    now = int(round(time.time() * 1000))
    out.write(struct.pack("!LL", now / 1000, (now % 1000) * 1000))
    out.write(struct.pack("!LL", len(data) + len(_ETH_DATA), len(data) + len(_ETH_DATA)))
    # Write the ETH header
    out.write(_ETH_DATA)
    # and the data
    out.write(data)
    out.flush()

def bitrev(data):
    return ((data << 7) & 0x80) | ((data << 5) & 0x40) | (data << 3) & 0x20 | (data << 1) & 0x10 | (data >> 7) & 0x01 | (data >> 5) & 0x02 | (data >> 3) & 0x04 | (data >> 1) & 0x08

class CCITT_CRC:
    crc = 0

    def addBitrev(self, data):
        self.add(bitrev(data))

    def getCRCLow(self):
        return bitrev(self.crc & 0xff);

    def getCRCHi(self):
        return bitrev(self.crc >> 8);

    def add(self, data):
        newCrc = ((self.crc >> 8) & 0xff) | (self.crc << 8) & 0xffff
        newCrc ^= (data & 0xff)
        newCrc ^= (newCrc & 0xff) >> 4
        newCrc ^= (newCrc << 12) & 0xffff
        newCrc ^= (newCrc & 0xff) << 5
        self.crc = newCrc & 0xffff
        return self.crc

def usage():
    print 'sniff.py [-s] [-c <channel>] [-t <time-in-seconds>] [-o <outputfile>] [-a host] [-p port]'

#
# Read command line arguments
#
outfile = "capture.pcap"
channel = None
end_time = None
host = "localhost"
port = 9999
try:
    argv = sys.argv[1:]
    opts, args = getopt.getopt(argv,"hso:c:t:a:p:")
except getopt.GetoptError as e:
    sys.stderr.write(str(e) + '\n')
    usage()
    sys.exit(2)
for opt, arg in opts:
    if opt == '-h':
        usage()
        print "Connect to a native border router and use it as a radio sniffer."
        print ""
        print "Example to run with Wireshark for 320 seconds:"
        print " ",sys.argv[0],"-s -t 320 | wireshark -k -i -"
        sys.exit()
    elif opt == "-o":
        outfile = arg
    elif opt == "-s":
        outfile = None
    elif opt == "-c":
        channel = arg
    elif opt == "-t":
        end_time = time.time() + int(arg)
    elif opt == "-a":
        host = arg
    elif opt == "-p":
        port = int(arg)

if outfile != None:
    out = open(outfile, "w")
else:
    out = sys.stdout

init_pcap(out)
con = serialradio.SerialRadioConnection()
con.connect(host=host, port=port)

if outfile != None:
    sys.stderr.write("Configuring radio for sniffing (border router will not work during capture)\n")
    sys.stderr.write("Capturing " + str(outfile) + "\n")
    #con.set_debug(True)
if channel != None:
    con.send("!C" + struct.pack("B", int(channel)))
con.send("!m\002")

while True:
    if end_time is not None and end_time < time.time():
        break

    data = con.get_next_block(10)
    if data != None and data.serial_data != None:
        crc = CCITT_CRC()
        #print "Received:", binascii.hexlify(data.serial_data[2:])
        for b in data.serial_data[2:]:
            crc.addBitrev(ord(b))
        new_data = data.serial_data[2:] + struct.pack("BB", crc.getCRCHi(), crc.getCRCLow())
        export_packet_data(out, new_data)

out.close()
con.close()
sys.stderr.write("[connection closed]\n");
