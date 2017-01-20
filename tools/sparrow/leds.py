#!/usr/bin/env python
#
# Copyright (c) 2015-2016, SICS, Swedish ICT
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
# Author: Niclas Finne, nfi@sics.se
#
# A simple Python utility to control the leds on a device using the
# Sparrow Application Layer.
#

import tlvlib, sys, struct, time, binascii

def usage():
    print "Usage: leds [-v] <host> [led [state]]"
    print "Usage: leds [-v] <host> [get,set,clear,toggle,control [leds mask]]"
    exit()

arg=1
verbose = False
while len(sys.argv) > arg:
    if sys.argv[arg] == "-v":
        verbose = True
    elif sys.argv[arg] == "-h":
        usage()
    else:
        break
    arg += 1

if len(sys.argv) <= arg:
    usage()

host = sys.argv[arg]
arg += 1

led = None
state = None
op = None
if arg < len(sys.argv):
    if sys.argv[arg] == "get" or sys.argv[arg] == "control":
        op = tlvlib.VARIABLE_LED_CONTROL
    elif sys.argv[arg] == "set":
        op = tlvlib.VARIABLE_LED_SET
    elif sys.argv[arg] == "clear":
        op = tlvlib.VARIABLE_LED_CLEAR
    elif sys.argv[arg] == "toggle":
        op = tlvlib.VARIABLE_LED_TOGGLE

if op is not None:
    arg += 1

if arg < len(sys.argv):
    led = tlvlib.decodevalue(sys.argv[arg])
    arg += 1

    if op is None and led <= 0:
        print "The led number must be positive"
        usage()

if op is None and arg < len(sys.argv):
    state = tlvlib.decodevalue(sys.argv[arg])
    arg += 1

if arg < len(sys.argv):
    usage()

instance = tlvlib.find_instance_with_type(host, tlvlib.INSTANCE_LEDS_GENERIC, verbose)
if not instance:
    print "Could not find a leds instance in", host
    exit()

ts = []

# Control leds via bit mask (only on/off)
if op is not None:
    if led is not None:
        t = tlvlib.create_set_tlv32(instance, op, led)
        ts.append(t)
    t = tlvlib.create_get_tlv32(instance, tlvlib.VARIABLE_LED_CONTROL)
    ts.append(t)
    enc,tlvs = tlvlib.send_tlv(ts, host)
    t = tlvs[len(tlvs) - 1]
    if t.error == 0:
        print "Current leds:",bin(t.int_value)
    exit()

# Control leds via states (supports multicolor leds)
VARIABLE_LED_STATE = 0x105
if state is not None:
    t = tlvlib.create_set_vector_tlv(instance, VARIABLE_LED_STATE,
                                     tlvlib.SIZE32, led - 1, 1,
                                     struct.pack("!L", state))
    ts.append(t)

if led is not None:
    t = tlvlib.create_get_vector_tlv(instance, VARIABLE_LED_STATE,
                                     tlvlib.SIZE32, led - 1, 1)
else:
    t = tlvlib.create_get_vector_tlv(instance, VARIABLE_LED_STATE,
                                     tlvlib.SIZE32, 0, 32)
ts.append(t)

enc,tlvs = tlvlib.send_tlv(ts, host)
t = tlvs[len(tlvs) - 1]
if t.error == 0:
    for r in range(0, t.element_count):
        state, = struct.unpack("!L", t.value[r * 4:(r + 1) * 4])
        print " Led",(t.element_offset + r + 1),"is in state",state
