#!/usr/bin/env python
#
# Copyright (c) 2013-2016, SICS, Swedish ICT
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
#         Niclas Finne, nfi@sics.se
#
# TLV Library
# A small Python library for use in applications using Yanzi Application Layer
#

import binascii
from ctypes import *
import socket
import struct
import sys
import time
import string
from math import exp

DEBUG = False
# Some constants
TLV_GET_REQUEST    = 0
TLV_GET_RESPONSE   = 1
TLV_SET_REQUEST    = 2
TLV_SET_RESPONSE   = 3
TLV_EVENT_REQUEST  = 6
TLV_EVENT_RESPONSE = 7

SIZE32             = 0
SIZE64             = 1
SIZE128            = 2
SIZE256            = 3
SIZE512            = 4

ENC_PAYLOAD_TLV    = 1
ENC_PAYLOAD_SERIAL = 8

ENC_FP_NONE        = 0  # 0 bytes
ENC_FP_DEVID       = 1  # 8 bytes
ENC_FP_FIP4        = 2  # 4 bytes
ENC_FP_LENOPT      = 3  # 4 bytes
ENC_FP_DID_AND_FP  = 4  # 16 bytes

ENC_FP_LENOPT_OPTION_CRC = 1
ENC_FP_LENOPT_OPTION_SEQNO_CRC = 2

CRC_MAGIC_REMAINDER = 0x2144DF1C

# some defaults
UDP_IP = "127.0.0.1"
UDP_PORT = 49111

# Some product / instance ID's and instance variables
PRODUCT_DECIBEL                   = 0x0090DA0301010510L
PRODUCT_CYCLOPS                   = 0x0090DA03010104B0L

INSTANCE_GPIO                     = 0x0090DA0302010013L
INSTANCE_BORDER_ROUTER            = 0x0090DA0302010014L
INSTANCE_ROUTER                   = 0x0090da0302010015L
INSTANCE_MOTION_GENERIC           = 0x0090DA0302010016L
INSTANCE_CO2                      = 0x0090DA0302010017L
INSTANCE_TEMPHUM_GENERIC          = 0x0090DA0302010018L
INSTANCE_TEMP_GENERIC             = 0x0090DA0302010019L
INSTANCE_LAMP                     = 0x0090DA030201001AL
INSTANCE_ACCELEROMETER            = 0x0090DA030201001BL
INSTANCE_RANGING_GENERIC          = 0x0090DA030201001CL
INSTANCE_BUTTON_GENERIC           = 0x0090DA030201001DL
INSTANCE_LEDS_GENERIC             = 0x0090DA030201001EL
INSTANCE_LIGHT_GENERIC            = 0x0090da0302010020L
INSTANCE_AMBIENT_RGB_GENERIC      = 0x0090da0302010023L
INSTANCE_SOUND_GENERIC            = 0x0090da0302010021L
INSTANCE_IMAGE                    = 0x0090da0303010010L
INSTANCE_SHT20                    = 0x0090DA0303010011L
INSTANCE_PTCTEMP                  = 0x0090DA0303010012L
INSTANCE_ENERGY_METER             = 0x0090da0303010013L
INSTANCE_RADIO                    = 0x0090DA0303010014L
INSTANCE_SLEEP                    = 0x0090DA0303010017L
INSTANCE_POWER_SINGLE             = 0x0090DA0303010021L
INSTANCE_BORDER_ROUTER_MANAGEMENT = 0x0090DA0303010022L
INSTANCE_NETWORK_STATISTICS       = 0x0090DA0303010023L

# variables available in all instances
VARIABLE_OBJECT_TYPE = 0x000
VARIABLE_OBJECT_ID = 0x001
VARIABLE_OBJECT_LABEL = 0x002
VARIABLE_NUMBER_OF_INSTANCES = 0x003
VARIABLE_OBJECT_SUB_TYPE = 0x004
VARIABLE_EVENT_ARRAY = 0x005

# variables used in chassis instance
VARIABLE_IDENTIFY_CHASSIS = 0x0e4
VARIABLE_RESET_CAUSE = 0x0e5

# variables used in image instances and flash
VARIABLE_FLASH                 =  0x100
VARIABLE_WRITE_CONTROL         =  0x101
VARIABLE_IMAGE_START_ADDRESS   =  0x102
VARIABLE_IMAGE_MAX_LENGTH      =  0x103
VARIABLE_IMAGE_TYPE            =  0x104
VARIABLE_IMAGE_STATUS          =  0x105
VARIABLE_IMAGE_VERSION         =  0x106
VARIABLE_IMAGE_LENGTH          =  0x107
VARIABLE_IMAGE_CRC32           =  0x108

FLASH_WRITE_CONTROL_ERASE        = 0x911
FLASH_WRITE_CONTROL_WRITE_ENABLE = 0x23

HARDWARE_RESET_KEY               = 0x38f0000

# variables used in sleep instance
VARIABLE_SLEEP_AWAKE_TIME_WHEN_NO_ACTIVITY = 0x101

IMAGE_STATUS_OK		   = 0x00
IMAGE_STATUS_BAD_CHECKSUM  = 0x01
IMAGE_STATUS_BAD_TYPE	   = 0x02
IMAGE_STATUS_BAD_SIZE	   = 0x04
IMAGE_STATUS_NEXT_BOOT	   = 0x08
IMAGE_STATUS_WRITABLE	   = 0x10
IMAGE_STATUS_ACTIVE	   = 0x20
IMAGE_STATUS_ERASED	   = 0x40

IMAGE_REVISION_TYPE = {
    0x00: "branch",
    0x01: "pre",
    0x02: "RC",
    0x03: "LD"
}

IMAGE_DATE_TYPE = {
    0x00: "snapshot",
    0x01: "untagged",
    0x02: "reserved",
    0x03: "reserved"
}

RESET_CAUSE_NAME = {
    0x00: "NONE",
    0x01: "LOCKUP",
    0x02: "DEEP SLEEP",
    0x03: "SOFTWARE",
    0x04: "WATCHDOG",
    0x05: "HARDWARE",
    0x06: "POWER FAIL",
    0x07: "COLD START"
}

RESET_EXT_CAUSE_NAME = {
    0x00: "NONE",
    0x01: "NORMAL",
    0x02: "WATCHDOG"
}

TLV_OP_NAME = {
    0: "GET REQUEST",
    1: "GET RESPONSE",
    2: "SET REQUEST",
    3: "SET RESPONSE",
    6: "EVENT REQUEST",
    7: "EVENT RESPONSE"
}

TLV_OP_SHORT_NAME = {
    0: "G",
    1: "GR",
    2: "S",
    3: "SR",
    6: "E",
    7: "ER"
}

TLV_ERROR_NAME = {
     0: "NO ERROR",
     1: "UNKNOWN VERSION",
     2: "UNKNOWN VARIABLE",
     3: "UNKNOWN INSTANCE",
     4: "UNKNOWN OP CODE",
     5: "UNKNOWN ELEMENT SIZE",
     6: "BAD NUMBER OF ELEMENTS",
     7: "TIMEOUT",
     8: "DEVICE BUSY",
     9: "HARDWARE ERROR",
    10: "BAD LENGTH",
    11: "WRITE ACCESS DENIED",
    12: "UNKNOWN BLOB COMMAND",
    13: "NO VECTOR ACCESS",
    14: "UNEXPECTED RESPONSE",
    15: "INVALID VECTOR OFFSET",
    16: "INVALID ARGUMENT",
    17: "READ ACCESS DENIED",
    18: "UNPROCESSED TLV"
}

SENSOR_DATA_STATUS_NAME = {
     0: "equals",
     1: "less than",
     2: "greater than",
     3: "value is status"
}

# variables in instance 0
VARIABLE_HARDWARE_RESET = 0x0ca
VARIABLE_UNIT_BOOT_TIMER = 0x0c9
VARIABLE_SW_REVISION = 0x0cc
VARIABLE_CHASSIS_CAPABILITIES = 0x0e0
VARIABLE_BOOTLOADER_VERSION = 0x0e1
VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX = 0x0e3
VARIABLE_SLEEP_DEFAULT_AWAKE_TIME = 0x0d0
VARIABLE_CHASSIS_ACTIVITY_CYCLES_REQUEST = 0x0ec

# variables used for the netselect process
VARIABLE_UNIT_CONTROLLER_WATCHDOG = 0x0c0
VARIABLE_UNIT_CONTROLLER_STATUS   = 0x0c1
VARIABLE_UNIT_CONTROLLER_ADDRESS  = 0x0c2
VARIABLE_LOCATION_ID              = 0x0ce

# variables used for the radio instance
VARIABLE_RADIO_CHANNEL         = 0x100
VARIABLE_RADIO_PAN_ID          = 0x101
VARIABLE_RADIO_BEACON_RESPONSE = 0x102
VARIABLE_RADIO_MODE            = 0x103
VARIABLE_RADIO_RESET_CAUSE     = 0x10e
VARIABLE_RADIO_LINK_LAYER_KEY  = 0x200
VARIABLE_RADIO_LINK_LAYER_SECURITY_LEVEL = 0x201

# misc variables for specific devices
VARIABLE_CO2		= 0x100
VARIABLE_TEMPERATURE	= 0x100
VARIABLE_HUMIDITY	= 0x101

VARIABLE_ACCELEROMETER_REGISTER    = 0x100
VARIABLE_ACCELEROMETER_X           = 0x101
VARIABLE_ACCELEROMETER_Y           = 0x102
VARIABLE_ACCELEROMETER_Z           = 0x103
VARIABLE_ACCELEROMETER_MODE        = 0x104
VARIABLE_ACCELEROMETER_SENSITIVITY = 0x105
VARIABLE_ACCELEROMETER_TYPE        = 0x106

VARIABLE_RELAY_READ   = 0x100
VARIABLE_RELAY_WRITE  = 0x101
VARIABLE_GPOUT_OUTPUT = 0x101
VARIABLE_GPOUT_CONFIG = 0x106
VARIABLE_GPOUT_CAPABILITIES = 0x107
VARIABLE_GPIO_INPUT = 0x100
VARIABLE_GPIO_TRIGGER_COUNTER = 0x104

# For the LED control
VARIABLE_NUMBER_OF_LEDS = 0x100
VARIABLE_LED_CONTROL = 0x101
VARIABLE_LED_SET = 0x102
VARIABLE_LED_CLEAR = 0x103
VARIABLE_LED_TOGGLE = 0x104

# For the routing table
VARIABLE_TABLE_LENGTH = 0x100
VARIABLE_TABLE_REVISION = 0x101
VARIABLE_TABLE = 0x102
VARIABLE_NETWORK_ADDRESS = 0x103

# For the network statistics
VARIABLE_NSTATS_DATA = 0x106

# For the energy meter
VARIABLE_TOTAL_ENERGY_CONSUMED = 0x100
VARIABLE_POWER = 0x101
VARIABLE_VOLTAGE = 0x102
VARIABLE_CURRENT = 0x103
VARIABLE_TOTAL_ACTIVE_TIME = 0x104


NULL_TLV = binascii.unhexlify("0000")

# internal data
_TLV_HEADER_FORMAT = "!BBHBBBB"
_TLV_VECTOR_FORMAT = "!LL"
_TLV_HEADER_SIZE = struct.calcsize(_TLV_HEADER_FORMAT)
_TLV_VECTOR_SIZE = struct.calcsize(_TLV_VECTOR_FORMAT)

class EncapHeader:
    def __init__(self):
        self.version = 1
        self.payload_type = 0
        self.error = 0
        self.fpmode = 0
        self.ivmode = 0
        self.encap_format = "!BBBB"
        self.encap_size = struct.calcsize(self.encap_format)
        self.tot_size = self.encap_size
        self.device_id = None
        self.option = None
        self.length = None
        self.data = None
        self.serial_data = None
        self.crc_ok = None

    def pack(self):
        data = struct.pack(self.encap_format, self.version << 4,
                           self.payload_type, self.error,
                           self.fpmode << 4 | self.ivmode)
        if self.tot_size > self.encap_size:
            data += self.data
        if self.option == ENC_FP_LENOPT_OPTION_CRC:
            crc32 = binascii.crc32(data) & 0xffffffff
            crc32data = struct.pack("<L", crc32)
            data = data + crc32data
        return data

    def set_serial(self, data):
        self.payload_type = ENC_PAYLOAD_SERIAL
        self.fpmode = ENC_FP_LENOPT
        self.option = ENC_FP_LENOPT_OPTION_CRC
        optlen = struct.pack("!HH", ENC_FP_LENOPT_OPTION_CRC, len(data))
        self.data = optlen + data
        self.tot_size = len(self.data)

    def unpack(self, bindata):
        size = self.encap_size
        data = struct.unpack_from(self.encap_format, bindata)
        self.version = data[0] >> 4
        self.payload_type = data[1]
        self.error = data[2]
        self.fpmode = data[3] >> 4
        self.ivmode = data[3] & 0xf

        # if this is with fingerprint mode DEVID then read in the DEVID
        if self.fpmode == ENC_FP_DEVID:
            self.device_id = bindata[4:12]
            self.data = bindata[4:12]
            size = size + 8

        # LENOPT => 4 bytes data // Should also check CRC option!!!
        if self.fpmode == ENC_FP_LENOPT:
            self.option, self.length = struct.unpack_from("!HH", bindata, 4)
            size = size + 4

        if self.payload_type == ENC_PAYLOAD_SERIAL:
            self.serial_data = bindata[size : size + self.length]
            self.data = bindata[size : size + self.length]
            self.crc32, = struct.unpack_from("!L", bindata, size + self.length)
            self.crc_ok = binascii.crc32(bindata[0:size + self.length + 4]) & 0xffffffff == CRC_MAGIC_REMAINDER
#            print "Data: ", binascii.hexlify(self.serial_data)
#            print "CRC32", hex(binascii.crc32(bindata[0:size + self.length]) & 0xffffffff), " <=> ", hex(self.crc32)
#            print "CRCOK:", self.crc_ok

        self.tot_size = size

        return size

    def size(self):
        return self.tot_size

    def print_encap(self):
        print "  ENC version", self.version
        print "  ENC type", self.payload_type
        print "  ENC error", self.error
        print "  ENC FPMode", self.fpmode
        print "  ENC IVMode", self.ivmode
        if self.device_id != None:
            print "  ENC device_id", binascii.hexlify(self.device_id)
        if self.length != None:
            print "  ENC Option", self.option, " length: ", self.length
        if self.serial_data != None:
            print "  ENC Data:", binascii.hexlify(self.serial_data)
        if self.crc_ok != None:
            print "  CRC OK:", self.crc_ok

#
# TLVHeader for both regular TLVs and Vector TLVs.
#
class TLVHeader:
    def __init__(self):
        self.version = 0
        self.length = 2
        self.variable = 0
        self.instance = 0
        self.op = 0
        self.element_size = SIZE32
        self.error = 0
        self.element_offset = 0
        self.element_count = 0

        # Internal data
        self.data = None
        self.value = None
        self.is_null = False

    def pack(self):
        data = struct.pack(_TLV_HEADER_FORMAT,
                           ((self.version << 4) | (self.length >> 8)),
                           (self.length & 0xff), self.variable, self.instance,
                           self.op, self.element_size, self.error)

        # if vector - add vector data too
        if self.op >= 128:
            data = data + struct.pack(_TLV_VECTOR_FORMAT,
                                      self.element_offset,
                                      self.element_count)
        if self.data:
            data = data + self.data
        return data

    def unpack(self, bindata):
        if len(bindata) == 2 and bindata[0] == '\x00' and bindata[1] == '\x00':
            self.is_null = True
            self.length = 2
            return 2

        size = _TLV_HEADER_SIZE
        data = struct.unpack_from(_TLV_HEADER_FORMAT, bindata)
        self.version = (data[0] >> 4) & 0xff
        self.length = ((data[0] & 0xf) << 8) | data[1]
        self.variable = data[2]
        self.instance = data[3]
        self.op = data[4]
        self.element_size = data[5]
        self.error = data[6]

        # if vector - read vector data too
        if self.op >= 128:
            data = struct.unpack_from(_TLV_VECTOR_FORMAT, bindata, _TLV_HEADER_SIZE)
            size = size + _TLV_VECTOR_SIZE
            self.element_offset = data[0]
            self.element_count = data[1]
            self.is_vector = True

        # if get response set up data in TLV also!
        if self.op == TLV_GET_RESPONSE:
            self.data = bindata[self.size(): self.size() + 4 * (2** self.element_size)]
            if self.element_size == SIZE32:
                self.int_value, = struct.unpack("!l", self.data)
            if self.element_size == SIZE64:
                self.int_value, = struct.unpack("!q", self.data)
        elif self.op == (TLV_GET_RESPONSE | 128):
            self.data = bindata[self.size(): self.size() +
                                self.element_count * 4 * (2 ** self.element_size)]

        if self.length * 4 > self.size():
            print "**** TLV Warning - data in unexpected OP",self.op
            self.data = bindata[self.size() : self.length * 4]

        # assing value for backwards compliance
        self.value = self.data
        return self.size()

    def size(self):
        if self.is_null: return 2
        size = 0
        # if with data then add data size
        if self.data: size = len(self.data)
        # if with vector then add the extra vector header size
        if self.op >= 128: size = size + _TLV_VECTOR_SIZE
        return _TLV_HEADER_SIZE + size

    def print_tlv(self):
        print_tlv(self)

def parse_image_version(version):
    vtype = (version >>  6) & 0x03
    incr  =  version & 0x1f
    if version & 0x80000000L:
        # Date based
        year =  (version >> 17) & 0x1fff
        month = ((version >> 13) & 0x0f)
        day =   (version >>  8) & 0x1f
        v = IMAGE_DATE_TYPE.get(vtype, str(vtype)) + "-" + str(year) + str(month).zfill(2) + str(day).zfill(2)
        if incr > 0:
             v += chr(96 + incr)
    else:
        # Revision based
        major = (version >> 24) & 0x7f
        minor = (version >> 16) & 0xff
        patch = (version >>  8) & 0xff
        v = str(major) + "." + str(minor) + "." + str(patch)
        if vtype != 3 or incr > 0:
            v += IMAGE_REVISION_TYPE.get(vtype, str(vtype))
        if incr > 0:
            v += str(incr)
    return v

def parse_encap(data):
    encap = EncapHeader()
    encap.unpack(data)
    return encap

def parse_tlvs(data):
    tlvs = []
    if DEBUG: print "Received TLV: ", binascii.hexlify(data)
    while len(data) > 0:
        tlv = parse_tlv(data)
        tlvs = tlvs + [tlv]
        data = data[tlv.size():]
        # Assume end of TLVs when less than 3 bytes there...
        if len(data) < 3:
#            print "Found zero TLV??? with len:", len(data)
            break
    return tlvs

def parse_tlv(data):
    tlv = TLVHeader()
    tlv.unpack(data)
    return tlv

# create encap packet with TLV payload
def create_encap(tlv):
    e = EncapHeader()
    e.version = 1
    e.payload_type = ENC_PAYLOAD_TLV
    enc_header = e.pack()
    if DEBUG: print "Encap: ", binascii.hexlify(enc_header)
    if type(tlv) == list:
        tlv_header = ""
        for t in tlv:
            tlv_header = tlv_header + t.pack()
    else:
        tlv_header = tlv.pack()
    if DEBUG: print "TLV: ", binascii.hexlify(tlv_header)
    data = enc_header + tlv_header + NULL_TLV
    return data

def create_get_tlv(instance, var, element_size):
    return create_tlv(TLV_GET_REQUEST, instance, var, element_size)

def create_get_tlv32(instance, var):
    return create_tlv(TLV_GET_REQUEST, instance, var, SIZE32)

def create_get_tlv64(instance, var):
    return create_tlv(TLV_GET_REQUEST, instance, var, SIZE64)

def create_get_tlv128(instance, var):
    return create_tlv(TLV_GET_REQUEST, instance, var, SIZE128)

def create_get_tlv256(instance, var):
    return create_tlv(TLV_GET_REQUEST, instance, var, SIZE256)

def create_get_tlv512(instance, var):
    return create_tlv(TLV_GET_REQUEST, instance, var, SIZE512)

def create_set_tlv(instance, var, element_size, data):
    return create_tlv(TLV_SET_REQUEST, instance, var, element_size, data)

def create_set_tlv32(instance, var, data):
    return create_tlv(TLV_SET_REQUEST, instance, var, SIZE32, struct.pack("!L", data))

def create_tlv(op, instance, var, element_size, data=None):
    tlv = TLVHeader()
    tlv.op = op
    tlv.instance = instance
    tlv.variable = var
    tlv.element_size = element_size
    tlv.version = 0
# this needs to be checked against data's size
# length might in that case increase!
    if data: tlv.length = 2 + len(data) / 4
    else: tlv.length = 2
    tlv.data = data
    return tlv

def create_get_vector_tlv(instance, var, element_size, offset, element_count):
    return create_vector_tlv(TLV_GET_REQUEST, instance, var, element_size, offset, element_count)

def create_set_vector_tlv(instance, var, element_size, offset, element_count, data):
    return create_vector_tlv(TLV_SET_REQUEST, instance, var, element_size, offset, element_count, data)

def create_vector_tlv(op, instance, var, element_size, offset, element_count, data=None):
    tlv = TLVHeader()
    tlv.op = op | 128
    tlv.instance = instance
    tlv.variable = var
    tlv.element_size = element_size
    tlv.element_offset = offset
    tlv.element_count = element_count
# this needs to be checked against data's size
# length might in that case increase!
    if data: tlv.length = 4 + len(data) / 4
    else: tlv.length = 4
    tlv.data = data
    return tlv


def send_tlv(intlv, host=UDP_IP, port=UDP_PORT, timeout=1.0, show_error=True, retry=True, retries=1):
    data = create_encap(intlv)
    # This is a very quick hack to detect IPv6 address (given by xx:yy::...)
    if ":" in host:
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    else:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    if DEBUG:
        print "sending: ", binascii.hexlify(data)
    sock.settimeout(timeout)
    sock.sendto(data, (host, port))
    if retry and retries > 0:
        for i in range(retries):
            try:
                data, addr = sock.recvfrom(1024)
            except socket.timeout:
                if i >= retries - 1:
                    sock.sendto(data, (host, port))
                    data, addr = sock.recvfrom(1024)
    else:
            data, addr = sock.recvfrom(1024)
    sock.close()

    if DEBUG: print "Received: ", binascii.hexlify(data)
    enc = parse_encap(data)
    tlvs = parse_tlvs(data[enc.size():])
    if show_error:
        for tlv in tlvs:
            if tlv.error != 0:
                if hasattr(tlv, 'op') and hasattr(tlv, 'instance'):
                    print "  TLV ERROR",tlv.error,"(" + get_tlv_error_as_string(tlv.error) + ") for",get_tlv_op_as_name(tlv.op),"in instance",tlv.instance,"variable 0x%x"%tlv.variable
                else:
                    print "  TLV ERROR",tlv.error,"(" + get_tlv_error_as_string(tlv.error) + ")"
                    print_tlv(tlv)
            if (tlv.error > 17 or DEBUG):
                print "  received raw: ", binascii.hexlify(data)
                print "  raw TLV: ", binascii.hexlify(data[enc.size():])
                print "  received: ", enc.size(), tlv.size(), data[enc.size() + tlv.size():]
                print "  -----------"
                enc.print_encap()
                print "  -----------"
                print_tlv(tlv)
    return enc, tlvs

def print_tlvs(tlvs):
    for t in tlvs:
        print_tlv(t)

def print_tlv(tlv):
    if tlv.is_null:
        print "NULL TLV"
        return
    print "  TLV ver:", tlv.version, " op:", tlv.op, " instance:", tlv.instance, " var: 0x%x"%(tlv.variable), " length:", tlv.length, " error:", tlv.error
    if tlv.op >= 128:
        print "\telements:", tlv.element_count, " offset:", tlv.element_offset, " size:", 4 * (2 ** tlv.element_size)
    else:
        print "\telement size:", 4 * (2 ** tlv.element_size)
    if hasattr(tlv, 'int_value'):
        print "\tvalue:", tlv.int_value, "0x%x"%tlv.int_value
    elif hasattr(tlv, 'value'):
        if tlv.value:
            print "\tvalue:", binascii.hexlify(tlv.value)
        else:
            print "\tvalue:", tlv.value

def get_tlv_short_info(tlv):
    if type(tlv) == list:
        info = ""
        for t in tlv:
            info += _get_tlv_short_info(t)
        return info
    else:
        return _get_tlv_short_info(tlv)

def _get_tlv_short_info(t):
    if t is None or t.is_null:
        return "[null]"
    if t.element_size == SIZE32 and hasattr(t, 'int_value'):
        end = ":0x%03x"%t.int_value + "]"
    else:
        end = "]"
    if t.error != 0:
        end = "E" + str(t.error) + end
    return "[" + str(t.instance) + ":0x%03x"%t.variable + ":" + get_tlv_op_as_short_name(t.op) + end

def calc_sht20_checksum(data):
    POLYNOMIAL = 0x131
    crc = 0
    for byte_ctr in range(len(data)):
        crc ^= data[byte_ctr]
        for bit in range(8, 0, -1):
            if (crc & 0x80) != 0:
                crc = (crc << 1) ^ POLYNOMIAL
            else:
                crc = (crc << 1)
            crc = crc & 0xff
    return crc

def convert_sht20_temperature(raw_value):
    # Ignore CRC for now
    temperature = raw_value >> 8
    return -46.85 + 175.72 * float(temperature) / 65536

def convert_sht20_humidity(rawHumidity):
    # Ignore CRC for now
    humidity = rawHumidity >> 8
    # Convert to relative humidity above liquid water
    return -6 + 125 * float(humidity) / 65536

def convert_sht20_humidity_over_ice(rawHumidity, temperature):
    # Ignore CRC for now
    humidity = rawHumidity >> 8
    # Convert to relative humidity above liquid water
    rhw = -6 + 125 * float(humidity) / 65536
    # Convert to relative humidity above ice
    rhi = rhw * exp((17.62 * temperature) / (234.12 + temperature)) / exp((22.46 * temperature) / (272.62 + temperature))
    return rhi

def convert_ieee64_time(time):
    seconds = (time >> 32) & 0xffffffff
    nanoseconds = time & 0xffffffff
    return (seconds, nanoseconds)

def convert_ieee64_time_to_string(elapsed):
    seconds = (elapsed >> 32) & 0xffffffff
    nanoseconds = elapsed & 0xffffffff
    years = seconds // (365 * 24 * 60 * 60)
    seconds = seconds % (365 * 24 * 60 * 60)
    days = seconds // (24 * 60 * 60)
    seconds = seconds % (24 * 60 * 60)
    hours = seconds // (60 * 60)
    seconds = seconds % (60 * 60)
    minutes = seconds // (60)
    seconds = seconds % (60)
    milliseconds = nanoseconds // 1000000
    nanoseconds = nanoseconds % 1000000
    time = ""
    if years > 0 or time != "":
        time = str(years) + " years "
    if days > 0 or time != "":
        time += str(days) + " days "
    if hours > 0 or time != "":
        time += str(hours) + " hours "
    if minutes > 0 or time != "":
        time += str(minutes) + " min "
    time += str(seconds) + " sec " + str(milliseconds) + " msec"
    if nanoseconds > 0:
        time += " " + str(nanoseconds) + " nanoseconds"
    return time

def get_ieee64_time_as_short_string(elapsed):
    seconds = (elapsed >> 32) & 0xffffffff
    nanoseconds = elapsed & 0xffffffff
    days = seconds // (24 * 60 * 60)
    seconds = seconds % (24 * 60 * 60)
    hours = seconds // (60 * 60)
    seconds = seconds % (60 * 60)
    minutes = seconds // (60)
    seconds = seconds % (60)
    milliseconds = nanoseconds // 1000000
    return "{:d}:{:>02d}:{:>02d}:{:>02d}'{:>03d}".format(days, hours, minutes, seconds, milliseconds)

def get_start_ieee64_time_as_string(elapsed):
    seconds = (elapsed >> 32) & 0xffffffff
    start_time = time.time() - seconds
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(start_time))

def get_image_status_as_string(status):
    text = ""
    if (status & IMAGE_STATUS_ACTIVE) != 0:
        text += ",active"
    if (status & IMAGE_STATUS_WRITABLE) != 0:
        text += ",upgradable"
    if (status & IMAGE_STATUS_ERASED) != 0:
        text += ",erased"
    if (status & IMAGE_STATUS_BAD_CHECKSUM) != 0:
        text += ",bad checksum"
    if (status & IMAGE_STATUS_BAD_TYPE) != 0:
        text += ",bad type"
    if (status & IMAGE_STATUS_BAD_SIZE) != 0:
        text += ",bad size"
    if (status & IMAGE_STATUS_NEXT_BOOT) != 0:
        text += ",next boot"
    if len(text) > 0:
        return text[1:]
    return text

def get_tlv_error_as_string(error):
    return TLV_ERROR_NAME.get(error, str(error))

def get_tlv_op_as_name(op):
    return TLV_OP_NAME.get(op & 0x7f, str(op))

def get_tlv_op_as_short_name(op):
    return TLV_OP_SHORT_NAME.get(op & 0x7f, str(op))

def get_sensor_data_status(sensor_data):
    return (sensor_data >> 30) & 0x3

def get_sensor_data_value(sensor_data):
    return sensor_data & 0x3fffffffL

def get_sensor_data_status_as_string(status):
    return SENSOR_DATA_STATUS_NAME.get(status, str(status))

def get_reset_cause_as_string(reset_cause):
    name = RESET_CAUSE_NAME.get((reset_cause >> 8), str(reset_cause >> 8))
    if (reset_cause & 0xff) != 0:
        name += "," + RESET_EXT_CAUSE_NAME.get(reset_cause & 0xff, str(reset_cause & 0xff))
    return name

def convert_string(text):
    i = string.find(text, "\0")
    if i >= 0:
        return text[0:i]
    return text

def decodevalue(x):
    if x.startswith("0x"):
        return int(x[2:], 16)
    if x.startswith("#") or x.startswith("$"):
        return int(x[1:], 16)
    if x.startswith("0b"):
        return int(x[2:], 2)
    if x.startswith("0o"):
        return int(x[2:], 8)
    if x.startswith("0") and len(x) > 1:
        return int(x[1:], 8)
    return int(x)

def find_instance_with_type(host, product_type, verbose = False):
    t = create_get_tlv32(0, VARIABLE_NUMBER_OF_INSTANCES)
    enc,tlvs = send_tlv(t, host)
    num_instances = tlvs[0].int_value
    if verbose:
        print "Searching through",num_instances,"instances"
    t = []
    for i in range(num_instances):
        t.append(create_get_tlv64(i + 1, VARIABLE_OBJECT_TYPE))
    enc,tlvs = send_tlv(t, host, show_error = False)
    for i in range(num_instances):
        if verbose:
            print "\tInstance",(i + 1)," %016x"%tlvs[i].int_value
        if tlvs[i].int_value == product_type:
            return i + 1
    return None

def discovery(host, port=UDP_PORT):
    # get product type
    t1 = create_get_tlv64(0, VARIABLE_OBJECT_TYPE)
    # get product label
    t2 = create_get_tlv256(0, VARIABLE_OBJECT_LABEL)
    # get number of instances
    t3 = create_get_tlv32(0, VARIABLE_NUMBER_OF_INSTANCES)
    # get the boot timer
    t4 = create_get_tlv64(0, VARIABLE_UNIT_BOOT_TIMER)
    enc,tlvs = send_tlv([t1,t2,t3,t4], host, port, timeout=3.0)
    product_type = tlvs[0].int_value
    product_label = convert_string(tlvs[1].value)
    num_instances = tlvs[2].int_value
    boot_timer = tlvs[3].int_value
    instances = []
    for i in range(num_instances):
        t1 = create_get_tlv64(i + 1, VARIABLE_OBJECT_TYPE)
        t2 = create_get_tlv256(i + 1, VARIABLE_OBJECT_LABEL)
        enc,tlvs = send_tlv([t1,t2], host, port, show_error = False)
        instance_type = 0xffffffffffffffffL
        instance_label = "<failed to discover>"
        if tlvs[0].error == 0:
            instance_type = tlvs[0].int_value
        if tlvs[1].error == 0:
            instance_label = convert_string(tlvs[1].value)
        instances = instances + [(instance_type, instance_label, i + 1)]
    return ((product_label,product_type,boot_timer), instances)
