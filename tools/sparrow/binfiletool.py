#!/usr/bin/env python
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
#
import sys, binascii, argparse, re

parser = argparse.ArgumentParser(description='convert hex to binary - and add padding.')
parser.add_argument("-i", help="input file - stdin is default.")
parser.add_argument("-o", help="input file - stdout is default.")
parser.add_argument("-c", help="byte value for padding - 0xff is default.")
parser.add_argument("-p", help="number of pad bytes AFTER infile or with minus - the total size of the file to pad.")
parser.add_argument("-P", help="number of pad bytes BEFORE infile.")
parser.add_argument("-B", action="store_true", help="file is binary - no hex to bin conversion.")
parser.add_argument("-V", action="store_true", help="print version and exit.")

args = parser.parse_args()

if args.V:
    print "Sparrow binfile tool - Version 1.0"
    exit()

# setup the in and out files
infile = open(args.i, 'r') if args.i else sys.stdin
outfile = open(args.o, 'w') if args.o else sys.stdout

padc = chr(int(args.c if args.c else "0xff", 16))
pad_after = int(args.p if args.p else 0)
pad_before = int(args.P if args.P else 0)

data = infile.read()

if not args.B:
    data = re.sub(r'(?m)^#.*\n?', '', data)
    data = binascii.unhexlify(''.join(data.split()))

# pad at the end
if pad_after < 0:
    data = data + padc * ((-pad_after) - len(data))
else:
    data = data + padc * pad_after

# pad at start
data = padc * pad_before + data

# write the file
outfile.write(data)
