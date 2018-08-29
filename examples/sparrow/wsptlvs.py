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

import wsplugin, tlvlib, thread, deviceserver, json, struct, socket

# The plugin class
class TLVCommands(wsplugin.DemoPlugin):

    def get_commands(self):
        return ["tlvled", "tlvlamp", "tlvtemp"]

    def handle_command(self, wsdemo, cmds):
        if cmds[0] == "tlvled":
            ip = cmds[1]
            led = cmds[2]
            thread.start_new_thread(tlvled, (wsdemo, ip, led))
            return True
        elif cmds[0] == "tlvlamp":
            ip = cmds[1]
            led = cmds[2]
            thread.start_new_thread(tlvlamp, (wsdemo, ip, led))
            return True
        elif cmds[0] == "tlvtemp":
            ip = cmds[1]
            thread.start_new_thread(tlvtemp, (wsdemo, ip))
            return True
        return False

# Toggle a LED on a Yanzi IoT-U10 node (or other node with LEDs)
def tlvled(ws, ip, led):
    de = ws.get_device_manager().get_device(ip)
    if de and de.leds_instance:
        t1 = tlvlib.create_set_tlv32(de.leds_instance, tlvlib.VARIABLE_LED_TOGGLE, 1 << int(led))
        try:
            enc, tlvs = tlvlib.send_tlv(t1, ip)
            if tlvs[0].error == 0:
                print "LED ", led, " toggle."
            else:
                print "LED Set error", tlvs[0].error
        except socket.timeout:
            print "LED No response from node", ip
            ws.sendMessage(json.dumps({"error":"No response from node " + ip}))

# Toggle a LED on a Yanzi IoT-U10 node (or other node with LEDs)
def tlvlamp(ws, ip, led):
    de = ws.get_device_manager().get_device(ip)
    if de and de.lamp_instance:
        t1 = tlvlib.create_set_tlv32(de.lamp_instance, tlvlib.VARIABLE_LAMP_CONTROL, float(led)/100.0 * 0xffffffffL)
        try:
            enc, tlvs = tlvlib.send_tlv(t1, ip)
            if tlvs[0].error == 0:
                print "LAMP set to ", led, "%"
            else:
                print "LAMP Set error", tlvs[0].error
        except socket.timeout:
            print "LAMP No response from node", ip
            ws.sendMessage(json.dumps({"error":"No response from node " + ip}))


# Read the temperature from a Sparrow device
def tlvtemp(ws, ip):
    de = ws.get_device_manager().get_device(ip)
    if de:
        instance = de.get_instance(tlvlib.INSTANCE_TEMP_GENERIC)
        if instance:
            t1 = tlvlib.create_get_tlv32(instance, tlvlib.VARIABLE_TEMPERATURE)
            try:
                enc, tlvs = tlvlib.send_tlv(t1, ip)
                if tlvs[0].error == 0:
                    temperature = tlvs[0].int_value
                    if temperature > 100000:
                        temperature = round((temperature - 273150) / 1000.0, 2)
                    print "\tTemperature:",temperature,"(C)"
                    ws.sendMessage(json.dumps({"temp":temperature,"address":ip}))
                else:
                    print "\tTemperature error", tlvs[0].error
            except socket.timeout:
                print "Temperature - no response from node", ip
                ws.sendMessage(json.dumps({"error":"No response from node " + ip}))
