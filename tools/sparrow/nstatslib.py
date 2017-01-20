#!/usr/bin/env python
#
# Copyright (c) 2015-2016, Yanzi Networks AB.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#    3. Neither the name of the copyright holders nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
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
# nstatslibrary
# A small library for parsing data from the network statistics instance
#

import tlvlib, struct, binascii

NSTATS_TYPE_RPL         = 1
NSTATS_TYPE_PARENT_INFO = 2
NSTATS_TYPE_BEACONS     = 3
NSTATS_TYPE_NETSELECT   = 4
NSTATS_TYPE_RADIO       = 5
NSTATS_TYPE_CONFIG      = 6

class NstatsData:
    def __init__(self, data_type):
        self.data_type = data_type

class NstatsRpl(NstatsData):

    def __init__(self, data_type, bindata):
        NstatsData.__init__(self, data_type)
        self._bindata = bindata

    def seqno(self):
        seqno, = struct.unpack("!B", self._bindata[0:1])
        return seqno

    def free_routes(self):
        f, = struct.unpack("!B", self._bindata[1:2])
        return f

    def free_neighbors(self):
        f, = struct.unpack("!B", self._bindata[2:3])
        return f

    def parent_switches(self):
        s, = struct.unpack("!B", self._bindata[3:4])
        return s

    def parent(self):
        return self._bindata[4:8]

    def parent_as_string(self):
        return binascii.hexlify(self.parent())

    def parent_rank(self):
        r, = struct.unpack("!H", self._bindata[8:10])
        return r

    def parent_etx(self):
        r, = struct.unpack("!H", self._bindata[10:12])
        return r

    def dag_rank(self):
        r, = struct.unpack("!H", self._bindata[12:14])
        return r

    def __str__(self):
        return "NstatsRpl(" + str(self.seqno()) + "," + str(self.dag_rank()) + "," + self.parent_as_string() + ")"

class Nstats:

    def __init__(self, data):
        self.version,count = struct.unpack("!BB", data[0:2])
        self.unknown_type = None
        self.stats = []
        offset = 2
        for r in range(0, count):
            if offset >= len(data):
                # too little data
                break

            data_type, = struct.unpack("!B", data[2 + r:2 + r + 1])
            offset += 1

            if data_type == NSTATS_TYPE_RPL:
                # RPL info
                if offset + 24 > len(data):
                    break
                info = NstatsRpl(data_type, data[offset:offset + 24])
                self.stats.append(info)
                offset += 24
            else:
                # Unknown type
                self.unknown_type = data_type
                break

    def get_data_by_type(self, data_type):
        for d in self.stats:
            if d.data_type == data_type:
                return d
        return None

    def __str__(self):
        return "Nstats(" + `self.version` + "," + str(len(self.stats)) + "," + `self.unknown_type` + ")"

def create_nstats_tlv(instance):
    return tlvlib.create_get_vector_tlv(instance, tlvlib.VARIABLE_NSTATS_DATA,
                                        tlvlib.SIZE32, 0, 64)

def fetch_nstats(address, instance):
    t = create_nstats_tlv(instance)
    enc,tlvs = tlvlib.send_tlv(t, address)
    if enc.error == 0 and tlvs[0].error == 0:
        nstats = Nstats(tlvs[0].value)
        return nstats
    return None
