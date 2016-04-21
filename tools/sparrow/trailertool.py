#!/usr/bin/python
#
# Copyright (c) 2016, SICS, Swedish ICT
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the Institute nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# Author: Joakim Eriksson, joakime@sics.se
#
# Read out trailer info from a flash file. Or generate a trailer info
# and append to a file (to create a flash file).
#

import sys, binascii,datetime,argparse
magic = 0x2144DF1C
inc = 0

def get_version(inc):
    now = datetime.datetime.now()
    v = 0x80000000 | (now.year & 0x1fff) << 17 | (now.month & 0xf) << 13 | ((now.day & 0x1f) << 8) | 0x40
    v = v | (inc & 0x3f);
    version = chr(0x00) + chr(0x90) + chr(0xda) + chr(0x02) + chr((v >> 24) & 0xff) + chr((v >> 16) & 0xff) + chr((v >> 8) & 0xff) + chr(v & 0xff)
    return ''.join(version)

def get_crc(data):
#    crc32 = binascii.crc32(data[0:-4]) & 0xffffffff
    crc32 = binascii.crc32(data) & 0xffffffff
    return crc32

def rev32(crc32):
    return (crc32 & 0xff) << 24 | ((crc32 >> 24) & 0xff) | ((crc32 & 0xff00) << 8) | ((crc32 & 0x00ff0000) >> 8)


def clean_hex(data):
    if data.startswith('0x'):
        data = data[2:]
    if data.endswith('ULL'):
        data = data[:-3]
    return data

parser = argparse.ArgumentParser(description='Parse and create trailers.')
parser.add_argument("-vV", action="store_true", help="print version")
parser.add_argument("-vT", action="store_true", help="print type")
parser.add_argument("-vS", action="store_true", help="print image start")
parser.add_argument("-vC", action="store_true", help="print CRC")
parser.add_argument("-vP", action="store_true", help="print product type")
parser.add_argument("-v",  action="store_true", help="verbose output")
parser.add_argument("-V", help="image version");
parser.add_argument("-I", help="image increment");
parser.add_argument("-T", help="image type");
parser.add_argument("-A", help="image start address");
parser.add_argument("-P", help="product type");
parser.add_argument("-i", help="input file");

args = parser.parse_args()

if (args.i):
    file = open(args.i, 'r')
else:
    file = sys.stdin

if args.P:
    product = binascii.unhexlify(clean_hex(args.P))
if args.T:
    image_type = binascii.unhexlify(clean_hex(args.T))
if args.I:
    inc = int(args.I)
if args.V:
    if args.V == "now":
        version = get_version(inc)
if args.A:
    image_start = binascii.unhexlify(clean_hex(args.A))

data = file.read()

# only add if multiple things are set so that trailer can be created
add = ('image_start' in globals()) & ('product' in globals()) & ('version' in globals())

if args.v:
    print >> sys.stderr, "Read ",len(data), "bytes", " will add:", add

if not add:
    if (args.v):
        print "Trailer:", binascii.hexlify(data[-40:])
    # product type before the trailer
    pos = -40
    if (args.vP):
        print binascii.hexlify(data[pos : pos + 8]).upper()
    pos = pos + 8

    # type first in the trailer
    if (args.vT):
        print binascii.hexlify(data[pos : pos + 8]).upper()
    pos = pos + 8

    # version
    if (args.vV):
        print binascii.hexlify(data[pos : pos + 8]).upper()
    pos = pos + 8

    # start of image
    if (args.vS):
        print "0x" + (binascii.hexlify(data[pos : pos + 4]).upper())
    pos = pos + 4

    # ignore...
    pos = pos + 4
#    print "Magic        : " + binascii.hexlify(data[pos : pos + 4])
    pos = pos + 4
    fc = data[pos :]
    filecrc = (ord(fc[0]) << 24) | (ord(fc[1]) << 16) | (ord(fc[2]) << 8) | ord(fc[3])
    if (args.vC):
        print "0x" + (hex(rev32(filecrc))[2:]).upper()
        if(args.v):
            print "CRC from file:", binascii.hexlify(fc), hex(filecrc)
            print "Total CRC:", hex(get_crc(data))
            print "calc CRC:", hex(rev32(get_crc(data[:-4])))

#print "CRC32        :", hex(get_crc(data)), "=", hex(filecrc)
#version = get_version(0)
#print binascii.hexlify(version)

if add:
    if len(data) % 4 > 0:
        if args.v:
            print >> sys.stderr, "Padded with: ", 4 - (len(data) % 4), "bytes."
        data = data + '\0' * (4 - (len(data) % 4))
    # add image type and version and start address (8 + 8 + 4 bytes)
    trailer = '' + image_type + version + image_start
    # add the config and magic CRC value 4 + 4 bytes
    trailer = trailer + binascii.unhexlify('0000040851597466')
    if args.v:
        print >> sys.stderr, "Trailer:", binascii.hexlify(trailer), len(trailer)
    data = data + product + trailer
    crc = rev32(get_crc(data))
    data = data + chr((crc >> 24) & 0xff) + chr((crc >> 16) & 0xff) + chr((crc >> 8) & 0xff) + chr((crc >> 0) & 0xff)
    if args.v:
        print >> sys.stderr, "Trailer:", binascii.hexlify(data[-40:]), " CRC:", hex(get_crc(data)), " Size: ", len(data)
    sys.stdout.write(data)
