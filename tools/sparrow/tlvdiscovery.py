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
# TLV Discovery
# A small discovery tool for devices/applications using Yanzi Application Layer
#

import tlvlib, sys, struct, socket, binascii

verbose = False
arg = 1
host = "localhost"
instance = -1

if len(sys.argv) > 1 and sys.argv[1] == "-v":
    verbose = True
    arg = 2
if len(sys.argv) > arg:
    host = sys.argv[arg]
    print "HOST: ", host
    arg += 1

if len(sys.argv) > arg:
    instance = int(sys.argv[arg])

d = tlvlib.discovery(host)
print "Product label:", d[0][0], " type: %016x"%d[0][1], " instances:", len(d[1])
print "Booted at:",tlvlib.get_start_ieee64_time_as_string(d[0][2]),"-",tlvlib.convert_ieee64_time_to_string(d[0][2])

if verbose and instance <= 0:
    t1 = tlvlib.create_get_tlv128(0, tlvlib.VARIABLE_SW_REVISION)
    t2 = tlvlib.create_get_tlv32(0, tlvlib.VARIABLE_BOOTLOADER_VERSION)
    t3 = tlvlib.create_get_tlv32(0, tlvlib.VARIABLE_CHASSIS_CAPABILITIES)
    t4 = tlvlib.create_get_tlv32(0, tlvlib.VARIABLE_RESET_CAUSE)
    enc, tlvs = tlvlib.send_tlv([t1,t2,t3,t4], host)
    revision = ""
    if tlvs[3].error == 0:
        print "Last reset cause:", tlvlib.get_reset_cause_as_string(tlvs[3].int_value)
    if tlvs[0].error == 0:
        revision += "SW revision: " + tlvlib.convert_string(tlvs[0].value)
    if tlvs[1].error == 0:
        revision += "\tBootloader: " + str(tlvs[1].int_value)
    if tlvs[2].error == 0:
        revision += " %016x"%tlvs[2].int_value
    if revision != "":
        print revision

if verbose and instance <= 0:
    t0 = tlvlib.create_get_tlv32(0, tlvlib.VARIABLE_LOCATION_ID)
    enc,tlvs = tlvlib.send_tlv(t0, host, show_error=False)
    if tlvs[0].error == 0:
        print "Location id:", tlvs[0].int_value
    elif tlvs[0].error == 9:
        print "Location id: not set"
    elif tlvs[0].error != 2:
        print "Location id: #error",tlvlib.get_tlv_error_as_string(tlvs[0].error)

i = 1
for data in d[1]:
    print "Instance " + str(data[2]) + ": type: %016x"%data[0], " ", data[1]
    if instance >= 0 and instance != i:
        True
    elif data[0] == tlvlib.INSTANCE_IMAGE and verbose:
        t1 = tlvlib.create_get_tlv64(i, tlvlib.VARIABLE_IMAGE_VERSION)
        t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_IMAGE_STATUS)
        t3 = tlvlib.create_get_tlv64(i, tlvlib.VARIABLE_IMAGE_TYPE)
        enc, tlvs = tlvlib.send_tlv([t1,t2,t3], host)
        tlv = tlvs[0]
        tlvstatus = tlvs[1]
        print "\tVersion:    %016x"%tlv.int_value, "  ", tlvlib.parse_image_version(tlv.int_value), "inc:", str(tlv.int_value & 0x1f)
        print "\tImage Type: %016x"%tlvs[2].int_value,"   Status:", tlvlib.get_image_status_as_string(tlvstatus.int_value)
    elif data[0] == tlvlib.INSTANCE_LEDS_GENERIC:
        t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_LED_CONTROL)
        t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_NUMBER_OF_LEDS)
        enc, tlvs = tlvlib.send_tlv([t1,t2], host)
        print "\tleds are set to 0x%02x"%tlvs[0].int_value,"(" + str(tlvs[1].int_value) + " user leds)"

    elif data[0] == tlvlib.INSTANCE_BUTTON_GENERIC:
        t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_GPIO_INPUT)
        t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_GPIO_TRIGGER_COUNTER)
        enc, tlvs = tlvlib.send_tlv([t1,t2], host)
        if tlvs[0].int_value == 0:
            print "\tbutton is not pressed, has been pressed",tlvs[1].int_value,"times"
        else:
            print "\tbutton is pressed, has been pressed",tlvs[1].int_value,"times"
        if verbose:
            # Arm instance 0 and rearm the button instance
            t0 = tlvlib.create_set_vector_tlv(0, tlvlib.VARIABLE_EVENT_ARRAY,
                                              tlvlib.SIZE32, 0, 1,
                                              struct.pack("!L", 1))
            t1 = tlvlib.create_set_vector_tlv(i, tlvlib.VARIABLE_EVENT_ARRAY,
                                              tlvlib.SIZE32, 0, 2,
                                              struct.pack("!LL", 1, 2))
            enc, tlvs = tlvlib.send_tlv([t0,t1], host)

    elif data[0] == tlvlib.INSTANCE_GPIO:
        t = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_GPOUT_OUTPUT)
        enc, tlvs = tlvlib.send_tlv(t, host)
        tlv = tlvs[0]
        print "\tgpout set to", tlv.int_value
        t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_GPOUT_CONFIG)
        t2 = tlvlib.create_get_tlv64(i, tlvlib.VARIABLE_GPOUT_CAPABILITIES)
        enc, tlvs = tlvlib.send_tlv([t1,t2], host, show_error=False)
        if tlvs[0].error == 0:
            print "\tgpout start mode is 0x%02x"%tlvs[0].int_value + "    Capabilities: 0x%016x"%tlvs[1].int_value
    elif data[0] == tlvlib.INSTANCE_SHT20:
        t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_TEMPERATURE)
        t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_HUMIDITY)
        enc, tlvs = tlvlib.send_tlv([t1,t2], host)
        temperature = tlvlib.convert_sht20_temperature(tlvs[0].int_value)
        raw_humidity = tlvs[1].int_value
        humidity = tlvlib.convert_sht20_humidity(raw_humidity)
        humidityI = tlvlib.convert_sht20_humidity_over_ice(raw_humidity, temperature)
        print "\tTemperature:",round(temperature, 3),"(C)"
        print "\tHumidity   : " + str(round(humidity, 3)) + "%  Humidity (I): " + str(round(humidityI,3)) + "%"
    elif data[0] == tlvlib.INSTANCE_TEMPHUM_GENERIC:
        t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_TEMPERATURE)
        t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_HUMIDITY)
        enc, tlvs = tlvlib.send_tlv([t1,t2], host)
        temperature = tlvs[0].int_value
        humidity = tlvs[1].int_value
        if temperature > 100000:
            temperature = (temperature - 273150) / 1000.0
        print "\tTemperature:",round(temperature, 3),"(C)"
        humidity = humidity / 10000.0
        print "\tHumidity   :",str(round(humidity, 3)) + "%"
    elif data[0] == tlvlib.INSTANCE_TEMP_GENERIC:
        t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_TEMPERATURE)
        enc, tlvs = tlvlib.send_tlv([t1], host)
        if tlvs[0].error == 0:
            temperature = tlvs[0].int_value
            if temperature > 100000:
                temperature = (temperature - 273150) / 1000.0
            print "\tTemperature:",round(temperature, 3),"(C)"
    elif data[0] == tlvlib.INSTANCE_ENERGY_METER:
        t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_POWER)
        t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_VOLTAGE)
        t3 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_CURRENT)
        t4 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_TOTAL_ACTIVE_TIME)
        t5 = tlvlib.create_get_tlv64(i, tlvlib.VARIABLE_TOTAL_ENERGY_CONSUMED)
        enc, tlvs = tlvlib.send_tlv([t1,t2,t3,t4,t5], host, show_error=False)
        tlvpower = tlvs[0]
        tlvvoltage = tlvs[1]
        print "\tPower:             %8.2f (W)"%(tlvs[0].int_value / 1000.0)
        print "\tVoltage:           %8.2f (V)    Current: %8.2f (A)"%(tlvs[1].int_value / 1000.0,tlvs[2].int_value / 1000.0)
        if tlvs[3].error == 0:
            print "\tTotal active time: %8d (sec)"%(tlvs[3].int_value)
        print "\tTotal consumed: %11.2f (Ws)"%(tlvs[4].int_value / 1000.0)
    elif data[0] == tlvlib.INSTANCE_CO2:
        t = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_CO2)
        enc, tlvs = tlvlib.send_tlv(t, host)
        tlv = tlvs[0]
        if tlv.error == 0:
            print "\tCO2:",tlv.int_value,"(ppm)"
        else:
            print "\tCO2: no data"
    elif data[0] == tlvlib.INSTANCE_RADIO:
        t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_RADIO_PAN_ID)
        t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_RADIO_CHANNEL)
        t3 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_RADIO_MODE)

        enc, tlvs = tlvlib.send_tlv([t1,t2,t3], host)
        print "\tPan id:",tlvs[0].int_value,"(0x%04x)"%tlvs[0].int_value," Channel:",tlvs[1].int_value,"  Mode:",tlvs[2].int_value

    elif data[0] == tlvlib.INSTANCE_ROUTER:
        t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_TABLE_LENGTH)
        t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_TABLE_REVISION)
        t3 = tlvlib.create_get_tlv128(i, tlvlib.VARIABLE_NETWORK_ADDRESS)
        enc, tlvs = tlvlib.send_tlv([t1,t2, t3], host)
        tlv = tlvs[0]
        print "\tNetwork address: 0x" + binascii.hexlify(tlvs[2].value)
        print "\tRouting table size:",tlv.int_value," revision:",tlvs[1].int_value
        if tlv.int_value > 0:
            t = tlvlib.create_get_vector_tlv(i, tlvlib.VARIABLE_TABLE,
                                             tlvlib.SIZE512, 0, tlv.int_value)
            enc, tlvs = tlvlib.send_tlv(t, host)
            tlv = tlvs[0]
            for r in range(0, tlv.element_count):
                o = r * 64
                IPv6Str = socket.inet_ntop(socket.AF_INET6, tlv.value[o:o+16])
                IPv6LLStr = socket.inet_ntop(socket.AF_INET6, tlv.value[o+16:o+32])
                print "\t",(r + 1), IPv6Str, "->", IPv6LLStr
    elif data[0] == tlvlib.INSTANCE_MOTION_GENERIC:
        t = tlvlib.create_get_tlv128(i, 0x100)
        enc,tlvs = tlvlib.send_tlv(t, host)
        time, = struct.unpack("!q", tlvs[0].value[0:8])
        count, = struct.unpack("!q", tlvs[0].value[8:])
        print "\tMotion:",count,"Time:",tlvlib.convert_ieee64_time_to_string(time)
        if verbose:
            # Arm instance 0 and rearm the motion instance
            t0 = tlvlib.create_set_vector_tlv(0, tlvlib.VARIABLE_EVENT_ARRAY,
                                              tlvlib.SIZE32, 0, 1,
                                              struct.pack("!L", 1))
            t1 = tlvlib.create_set_vector_tlv(i, tlvlib.VARIABLE_EVENT_ARRAY,
                                              tlvlib.SIZE32, 0, 2,
                                              struct.pack("!LL", 1, 2))
            enc, tlvs = tlvlib.send_tlv([t0,t1], host)

    elif data[0] == tlvlib.INSTANCE_ACCELEROMETER:
        t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_ACCELEROMETER_MODE)
        t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_ACCELEROMETER_SENSITIVITY)
        enc,tlvs = tlvlib.send_tlv([t1,t2], host)
        print "\tAccelerometer mode:",tlvs[0].int_value,"Sensitivity:",tlvs[1].int_value
        if tlvs[0].int_value == 2:
            # Accelerometer is in orientation mode
            t1 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_ACCELEROMETER_X)
            t2 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_ACCELEROMETER_Y)
            t3 = tlvlib.create_get_tlv32(i, tlvlib.VARIABLE_ACCELEROMETER_Z)
            enc,tlvs = tlvlib.send_tlv([t1,t2,t3], host)
            print "\t\tX=" + str(tlvs[0].int_value),"Y=" + str(tlvs[1].int_value),"Z=" + str(tlvs[2].int_value)

    elif data[0] == tlvlib.INSTANCE_RANGING_GENERIC:
        t1 = tlvlib.create_get_tlv32(i, 0x101)
        t2 = tlvlib.create_get_tlv32(i, 0x102)
        enc,tlvs = tlvlib.send_tlv([t1,t2], host)
        print "\tDistance:",tlvs[1].int_value,"mm\tStatus:",tlvs[0].int_value
    elif data[0] == tlvlib.INSTANCE_PTCTEMP:
        t = tlvlib.create_get_tlv32(i, 0x100)
        enc,tlvs = tlvlib.send_tlv(t, host)
        if tlvs[0].error == 0:
            print "\tTemperature:",((tlvs[0].int_value - 273150) / 1000.0),"(C)"
    elif data[0] == tlvlib.INSTANCE_POWER_SINGLE:
        t1 = tlvlib.create_get_tlv32(i, 0x100)
        t2 = tlvlib.create_get_tlv256(i, 0x102)
        t3 = tlvlib.create_get_tlv32(i, 0x105)
        t4 = tlvlib.create_get_tlv32(i, 0x107)
        enc,tlvs = tlvlib.send_tlv([t1,t2,t3,t4], host)
        print "\tPower source: ",tlvlib.convert_string(tlvs[1].value),"(%d"%tlvs[0].int_value,"power sources)"
        print "\tPower voltage:",(tlvs[2].int_value / 1000.0),"V\tTemperature:",((tlvs[3].int_value - 273150)/1000.0),"C"
        if verbose:
            t1 = tlvlib.create_get_vector_tlv(i, 0x110, tlvlib.SIZE256, 0, 4)
            t2 = tlvlib.create_get_vector_tlv(i, 0x112, tlvlib.SIZE64, 0, 4)
            t3 = tlvlib.create_get_vector_tlv(i, 0x111, tlvlib.SIZE64, 0, 4)
            enc,tlvs = tlvlib.send_tlv([t1,t2,t3], host)
            for r in range(0, 4):
                n = tlvlib.convert_string(tlvs[0].data[r * 32:(r + 1) * 32])
                t, = struct.unpack("!q", tlvs[1].data[r * 8:(r + 1) * 8])
                c, = struct.unpack("!q", tlvs[2].data[r * 8:(r + 1) * 8])
                c /= 1000.0
                print "\t{:<14}{:>7.1f} uA ".format(n + ":",c),tlvlib.get_ieee64_time_as_short_string(t)
    elif data[0] == tlvlib.INSTANCE_LAMP:
        t1 = tlvlib.create_get_tlv32(i, 0x100)
        t2 = tlvlib.create_get_tlv32(i, 0x101)
        enc,tlvs = tlvlib.send_tlv([t1,t2], host)
        print "\tIntensity: 0x%x/%d"%(tlvs[0].int_value & 0xffffffffL,tlvs[1].int_value)
    elif data[0] == tlvlib.INSTANCE_NETWORK_STATISTICS:
        t1 = tlvlib.create_get_tlv32(i, 0x100)
        t2 = tlvlib.create_get_tlv64(i, 0x101)
        t3 = tlvlib.create_get_tlv32(i, 0x102)
        t4 = tlvlib.create_get_tlv32(i, 0x103)
        t5 = tlvlib.create_get_tlv32(i, 0x104)
        enc,tlvs = tlvlib.send_tlv([t1,t2,t3,t4,t5], host)
        version = tlvs[0].int_value
        capabilities = tlvs[1].int_value
        pushPeriod = tlvs[2].int_value
        pushTime = tlvs[3].int_value
        pushPort = tlvs[4].int_value
        cap_desc = ""
        if capabilities & 0x0800:
            if capabilities & 0x1000:
                cap_desc += " [sleeping SLEEP2]"
            else:
                cap_desc += " SLEEP2"
        if capabilities & 0x0100:
            if capabilities & 0x0200:
                cap_desc += " ROUTER"
            else:
                cap_desc += " [non-routing ROUTER]"
        if capabilities & 0x0400:
            cap_desc += " PUSH"
        print "\tVersion:",version,"\tCapabilities: 0x%x"%capabilities,cap_desc
        if pushPeriod > 0:
            print "\tPush period:",pushPeriod,"sec\tRemaining:",pushTime,"sec\tTo port:",pushPort

        t1 = tlvlib.create_get_tlv256(i, 0x107)
        enc,tlvs = tlvlib.send_tlv(t1, host, show_error=False)
        if tlvs[0].error == 0:
            data = tlvs[0].value
            version, = struct.unpack("!B", data[0:1])
            parent_switches, = struct.unpack("!H", data[2:4])
            time_since_switch, = struct.unpack("!L", data[4:8])
            parent = data[8:16]
            parent_rank, = struct.unpack("!H", data[16:18])
            parent_metric, = struct.unpack("!H", data[18:20])
            print
            print "\tParent:", "0x" + binascii.hexlify(parent), "link-metric:", parent_metric," rank:", parent_rank
            print "\tChurn:", parent_switches, " Last parent switch", time_since_switch, "seconds ago"

        if verbose:
            t1 = tlvlib.create_get_vector_tlv(i, 0x106, tlvlib.SIZE32, 0, 64)
            enc,tlvs = tlvlib.send_tlv([t1], host)
            data_version,data_count,data_type = struct.unpack("!BBB", tlvs[0].value[0:3])
            print
            if data_version != 0:
                print "\tUnsupported data version:",data_version
            elif data_count == 0:
                print "\tNo data"
            elif data_type != 1:
                print "\tUnsupported data type ",data_type
            else:
                seqno, = struct.unpack("!B", tlvs[0].value[3:4])
                free_routes, = struct.unpack("!B", tlvs[0].value[4:5])
                free_neighbors, = struct.unpack("!B", tlvs[0].value[5:6])
                churn, = struct.unpack("!B", tlvs[0].value[6:7])
                parent = tlvs[0].value[7:11]
                rank, = struct.unpack("!H", tlvs[0].value[11:13])
                rtmetric, = struct.unpack("!H", tlvs[0].value[13:15])
                myrank, = struct.unpack("!H", tlvs[0].value[15:17])
                print "\tRPL Rank:", myrank," Churn:",churn," Free neighbors:",free_neighbors," Free routes:",free_routes
                print "\tDefault route:","0x" + binascii.hexlify(parent)," link-metric:",rtmetric," rank:",rank
    elif data[0] == tlvlib.INSTANCE_BORDER_ROUTER_MANAGEMENT:
        t1 = tlvlib.create_get_tlv256(i, 0x100)
        t2 = tlvlib.create_get_tlv256(i, 0x101)
        t3 = tlvlib.create_get_tlv128(i, 0x105)
        t4 = tlvlib.create_get_tlv32(i, 0x106)
        enc,tlvs = tlvlib.send_tlv([t1,t2,t3,t4], host)
        interface_name_label = tlvs[0].value
        serial_name_label = tlvs[1].value
        print "\tSerial device name:", serial_name_label,"  Interface device name:", interface_name_label
        if hasattr(tlvs[2], 'value'):
            if hasattr(tlvs[3], 'int_value'):
                print "\tRadio SW revision:",tlvlib.convert_string(tlvs[2].value),"\tBootloader version:",tlvs[3].int_value
            else:
                print "\tRadio SW revision:",tlvlib.convert_string(tlvs[2].value)

    i = i + 1
