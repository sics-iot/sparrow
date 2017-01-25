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
#         Niclas Finne, nfi@sics.se
#

import sys, binascii, datetime, argparse, zipfile, tlvlib, struct, socket

def get_flash(zip, image):
    findstr = str(image) + ".flash"
    infolist = zip.infolist()
    for info in infolist:
        if info.filename.find(findstr) > 0:
            return info
    return None

def get_manifest_data(zip):
    f = zip.open('META-INF/MANIFEST.MF')
    lines = f.readlines()
    image = 0
    data = {}
    for l in lines:
        kv = l.lower().split(':')
        if len(kv) != 2:
            image = 0
        else:
            k = kv[0].strip()
            v = kv[1].strip()
            if k == 'producttype':
                data['producttype'] = v
            elif k == 'name':
                if '1.flash' in v:
                    image = 1
                elif '2.flash' in v:
                    image = 2
                else:
                    image = 0
            elif k == 'imagetype':
                if image != 0:
                    data['image.' + str(image) + '.type'] = v
    f.close()
    return data

def get_image_crc32(instance, host, port):
    t = tlvlib.create_get_tlv32(instance, tlvlib.VARIABLE_IMAGE_CRC32)
    enc,tlvs = tlvlib.send_tlv(t, host, port)
    if tlvs[0].error == 0:
        return tlvs[0].int_value
    return None

def verify_image_crc32(instance, data, host, port):
    image_crc32 = get_image_crc32(instance, host, port)
    if not image_crc32:
        print "ERROR failed to get CRC32"
        return False
    data_crc32 = binascii.crc32(data) & 0xffffffff
    if image_crc32 == data_crc32:
        print "Image CRC32 OK!"
        return True
    print "ERROR CRC32: image CRC32:",image_crc32,"firmware CRC32:",data_crc32

def get_image_status(instance, host, port):
    t = tlvlib.create_get_tlv32(instance, tlvlib.VARIABLE_IMAGE_STATUS)
    enc,tlvs = tlvlib.send_tlv(t, host, port)
    if tlvs[0].error == 0:
        return tlvs[0].int_value
    return None

def send_upgrade(segment, size, instance, host, port):
    print "Upgrading ", segment[0], instance, len(segment[1]), " at ", segment[0] * size, "\b" * 35,
    sys.stdout.flush()
    t1 = tlvlib.create_set_tlv32(instance, tlvlib.VARIABLE_WRITE_CONTROL,
                                 tlvlib.FLASH_WRITE_CONTROL_WRITE_ENABLE)
    t2 = tlvlib.create_set_vector_tlv(instance, tlvlib.VARIABLE_FLASH,
                                      tlvlib.SIZE32, segment[0] * size / 4, len(segment[1]) / 4, segment[1])
    try:
        enc,tlvs = tlvlib.send_tlv([t1,t2], host, port, 1.5)
    except socket.timeout:
        return False
    #tlvlib.print_tlvs(tlvs)
    return True

def create_segments(data, size):
    segments = {}
    i = 0
    while len(data) > 0:
        segments[i] = [i,data[0:size]]
        i = i + 1
        data = data[size:]
    return segments

def do_erase(instance, host, port):
    # create Erase TLV
    t1 = tlvlib.create_set_tlv32(instance, tlvlib.VARIABLE_WRITE_CONTROL, tlvlib.FLASH_WRITE_CONTROL_ERASE)
    try:
        enc,tlvs = tlvlib.send_tlv(t1, host, port, 2.5)
    except socket.timeout:
        return False
    return tlvs[0].error == 0

def do_reboot(host, port, image=0):
    if image == 0:
        t = tlvlib.create_set_tlv32(0, tlvlib.VARIABLE_HARDWARE_RESET, 1)
    else:
        t = tlvlib.create_set_tlv32(0, tlvlib.VARIABLE_HARDWARE_RESET,
                                    tlvlib.HARDWARE_RESET_KEY | image)
    try:
        tlvlib.send_tlv(t, host, port, show_error=False, retry=False, timeout=0.2)
    except socket.timeout:
        # A timeout is expected since the node should reboot
        pass
    return True

def do_upgrade(data, instance, host, port, block_size, retry_passes=50):
    to_upgrade = create_segments(data, block_size)

    print "Got", len(to_upgrade.keys()), "segments."

    i = 0
    while i < retry_passes and len(to_upgrade.keys()) > 0:
        i += 1
        print "Writing", i, segments, "left to go.", "\b" * 35,
        sys.stdout.flush()
        new_upgrade = {}
        for udata in to_upgrade:
            if not send_upgrade(to_upgrade[udata], block_size, instance, host, port):
                new_upgrade[udata] = to_upgrade[udata]
        to_upgrade = new_upgrade
    print
    return len(to_upgrade.keys()) == 0

block_size = 512
port = tlvlib.UDP_PORT
firmware = None
verbose = False

parser = argparse.ArgumentParser(description='Over-the-Air update tool.')
parser.add_argument("-a", help="host")
parser.add_argument("-i", help="input file (image archive)")
parser.add_argument("-k", action="store_true",
                    help="keep running after upgrade (do not reboot)")
parser.add_argument("-s", action="store_true", help="only show status")
parser.add_argument("-p", help="port (default: %(default)s)", default=port)
parser.add_argument("-b", help="block size (default: %(default)s)",
                    default=block_size)
parser.add_argument("-v", action="store_true", help="verbose output")

args = parser.parse_args()
keep_running = False

if args.i:
    firmware = args.i

if args.a:
    host = args.a
else:
    print "Please specify host address"
    parser.print_help()
    exit()

if args.p:
    port = int(args.p)

if args.b:
    block_size = int(args.b)

if args.v:
    verbose = True

if args.k:
    keep_running = True

if not args.s:
    if not firmware:
        print "Specify image file"
        parser.print_help()
        exit()

    file = open(firmware, 'r')
    zip = zipfile.ZipFile(file)

try:
    d = tlvlib.discovery(host, port)
except socket.timeout:
    print "Failed to discover the device. Probably the device is offline or sleeping."
    exit()
producttype = "%016x"%d[0][1]
print "---- Upgrading ----"
print "Product label:", d[0][0]," type:",producttype,"instances:", len(d[1])
print "Booted at:",tlvlib.get_start_ieee64_time_as_string(d[0][2]),"-",tlvlib.convert_ieee64_time_to_string(d[0][2])

i = 1
upgrade = 0
for data in d[1]:
    if data[0] == tlvlib.INSTANCE_IMAGE:
        print "Instance " + str(data[2]) + ": type: %016x"%data[0], " ", data[1]
        t1 = tlvlib.create_get_tlv64(i, tlvlib.VARIABLE_IMAGE_VERSION)
        t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_IMAGE_STATUS)
        t3 = tlvlib.create_get_tlv64(i, tlvlib.VARIABLE_IMAGE_TYPE)
        enc, tlvs = tlvlib.send_tlv([t1,t2,t3], host, port)
        tlv = tlvs[0]
        tlvstatus = tlvs[1]
        tlvtype = tlvs[2]
        print "\tVersion:    %016x"%tlv.int_value, "  ", tlvlib.parse_image_version(tlv.int_value), "inc:", str(tlv.int_value & 0x1f)
        print "\tImage Type: %016x"%tlvtype.int_value,"   Status:", tlvlib.get_image_status_as_string(tlvstatus.int_value)
        if (tlvstatus.int_value & tlvlib.IMAGE_STATUS_ACTIVE) == 0:
            upgrade = i
            label = data[1]
            upgrade_status = tlvstatus.int_value
            upgrade_version = tlv.int_value
            upgrade_type = binascii.hexlify(tlvtype.value)
            if label == "Primary firmware":
                upgrade_image = 1
            elif label == "Backup firmware":
                upgrade_image = 2
            else:
                upgrade_image = 0
    i = i + 1

if args.s:
    # Only status. Done.
    exit()

if upgrade == 0 or upgrade_image == 0:
    print "ERROR: no unused image found in the device"
    exit()

manifest = get_manifest_data(zip)
if manifest['producttype'] != producttype:
    if manifest['producttype'] == '0090da0301010482' and producttype == '0090da0302010014':
        # Sparrow serial radio has two product types: one for border router and
        # and one for serial radio. No need to warn for this.
        pass
    else:
        print "WARNING: different product type in firmware file and in device:",manifest['producttype'],"!=",producttype

imagetype = manifest['image.' + str(upgrade_image) + '.type']
if imagetype != upgrade_type:
    print "ERROR: wrong image type in firmware file:",imagetype,"!=",upgrade_type
    exit()

zfile = get_flash(zip, upgrade_image)
if zfile is None:
    print "ERROR: could not find firmware data in firmware file"
    exit()

data = zip.read(zfile)
print "Upgrading instance", upgrade, label, "with image", zfile.filename," (" + str(len(data)) + " bytes)"
if (upgrade_status & tlvlib.IMAGE_STATUS_ERASED) == 0:
    print "Erasing image",upgrade
    i = 5
    while i >= 0:
         if not do_erase(upgrade, host, port):
             i = i - 1
             if i == 0:
                 print "ERROR: failed to erase image"
                 exit()
         else:
             break

if not do_upgrade(data, upgrade, host, port, block_size):
    print "ERROR: failed to write firmware file"
    exit()

upgrade_status = get_image_status(upgrade, host, port)
if not upgrade_status:
    print "ERROR: failed to verify upgrade status"
    exit()

if upgrade_status != tlvlib.IMAGE_STATUS_WRITABLE:
    print "ERROR: failed to upgrade image:", upgrade_status
    exit()

# No errors, status ok
print "New image OK. Rebooting to new image."
if not do_reboot(host, port):
    print "ERROR: failed to reboot to new image"
