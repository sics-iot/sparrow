#!/usr/bin/env python
#
# Copyright (c) 2017, SICS, Swedish ICT AB.
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
# Publish and listen on MQTT
#

import paho.mqtt.client as mqtt
import atexit, tlvlib

_client = None
_deviceserver = None

# Callback for connection response from server.
def on_connect(client, userdata, flags, rc):
    if rc != 0:
        print("MQTT: Connected with result code " + str(rc))
    client.subscribe("$SYS/#")
    client.subscribe("yanzi/#")

# Callback for published messages from the server.
def on_message(client, userdata, msg):
    if len(msg.topic) == 0:
        # Ignore empty topics
        pass
    elif len(msg.topic) > 0 and msg.topic[0] == '$':
        # Ignore system topics
        pass
    elif msg.topic.startswith("yanzi/"):
        handle_yanzi_topic(client, userdata, msg)
    else:
        print "MQTT ignoring:", msg.topic, str(msg.payload)

# Control a lamp using Sparrow
def _set_lamp(device, intensity):
    t0 = tlvlib.create_set_tlv32(device.lamp_instance, tlvlib.VARIABLE_LAMP_INTENSITY, intensity)
    t1 = tlvlib.create_get_tlv32(device.lamp_instance, tlvlib.VARIABLE_LAMP_INTENSITY)
    try:
        print "MQTT setting light in",device.did,"to 0x%08x"%intensity
        enc,tlvs = tlvlib.send_tlv([t0,t1], device.address)
        if enc.error != 0 or tlvs[0].error != 0:
            print "MQTT failed to set light in",device.did
        elif tlvs[1].error != 0:
            print "MQTT failed to read light in",device.did
        else:
            if tlvs[1].int_value == 0:
                device.lamp_is_on = False
            else:
                device.lamp_intensity = tlvs[1].int_value & 0xffffffff
                device.lamp_is_on = True
            return True
    except Exception as e:
        print "MQTT failed to set light in",device.did,e
    return False

# Handle the light topic
def handle_light(client, device, op, payload):
    if device.lamp_instance is None:
        print "MQTT",device.did,"is not a lamp!"
    elif op == "switch":
        print "PAYLOAD '" + payload + "'"
        if payload == "ON":
            if device.lamp_intensity is not None and device.lamp_intensity > 0:
                value = device.lamp_intensity
            else:
                value = 0xffffffff
        else:
            value = 0
        if _set_lamp(device, value):
            if device.lamp_is_on:
                payload = "ON"
            else:
                payload = "OFF"
            client.publish("yanzi/" + device.did + "/light/status", payload)
            return True
    elif op == "status":
        # Ignore status as we probably are the one that sent it
        return True
    else:
        print "MQTT OP",op,"not supported for",device.did
    return False

# Handle the brightness topic
def handle_brightness(client, device, op, payload):
    was_on = device.lamp_is_on
    if device.lamp_instance is None:
        print "MQTT",device.did,"is not a lamp!"
    elif op == "set":
        value = tlvlib.decodevalue(payload)
        value = tlvlib.convert_percent_to_intensity32(value)
        if _set_lamp(device, value):
            if device.lamp_is_on:
                value = tlvlib.convert_intensity32_to_percent(device.lamp_intensity)
            else:
                value = 0
            client.publish("yanzi/" + device.did + "/brightness/status", str(value))
            if was_on is not None and was_on != device.lamp_is_on:
                if device.lamp_is_on:
                    payload = "ON"
                else:
                    payload = "OFF"
                # The lamp has changed status
                client.publish("yanzi/" + device.did + "/light/status", payload)
            return True
    elif op == "status":
        # Ignore status as we probably are the one that sent it
        return True
    else:
        print "MQTT OP",op,"not supported for",device.did
    return False

# Handle Yanzi topics (for controlling Yanzi Network devices)
def handle_yanzi_topic(client, userdata, msg):
    global _deviceserver
    topic = msg.topic.split("/")
    if _deviceserver is None:
        # Not configured with device server
        return False
    if len(topic) < 4:
        print "MQTT ignoring:", msg.topic, str(msg.payload)
        return False
    did = topic[1]
    type = topic[2]
    op = topic[3]
    device = _deviceserver.get_device_by_did(did)
    payload = str(msg.payload)
    if not device:
        print "MQTT device with DID",did,"not found"
    elif type == "light":
        return handle_light(client, device, op, payload)
    elif type == "brightness":
        return handle_brightness(client, device, op, payload)
    else:
        print "MQTT type",type," not supported for",did
    return False

def start_mqtt(deviceserver, host='127.0.0.1', port=1883, user=None, password=None):
    global _client
    global _deviceserver
    _deviceserver = deviceserver
    _client = mqtt.Client()
    _client.on_connect = on_connect
    _client.on_message = on_message
    if user is not None and password is not None:
        _client.username_pw_set(user, password)
    _client.connect(host, port)
    atexit.register(stop_mqtt)
    _client.loop_start()
    print "MQTT started!"

def stop_mqtt():
    global _client
    if _client is not None:
        _client.loop_stop()
        _client.disconnect()
        print "MQTT stopped"

if __name__ == "__main__":
    start_mqtt()
    client.loop_forever()
