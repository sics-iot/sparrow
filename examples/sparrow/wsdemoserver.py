#!/usr/bin/env python
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
# Author: Joakim Eriksson, joakime@sics.se
#         Niclas Finne, nfi@sics.se
#
# Web Socket demo server
#

import sys
sys.path.append("../../tools/sparrow")

DEBUG = 0

import sys, subprocess, thread, string, tlvlib, socket, binascii
from SimpleWebSocketServer import WebSocket, SimpleWebSocketServer
import json, deviceserver, struct, wspserial, wsptlvs, wspnodes
import httpd

# Some global vaiables
websockets = []
nbr_address = None
device_manager = None

def send_response(ws, message):
    ws.sendMessage(json.dumps({"response":message}))

def send_error(ws, message):
    print "Sending error:",message
    ws.sendMessage(json.dumps({"error":message}))

def ping(ws, ip):
    ws.stop = False
    print "Pinging: ", ip
    p=subprocess.Popen(["ping6", "-c", "10", ip],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        while not ws.stop:
            line = p.stdout.readline()
            if line == '': break
            ws.sendMessage(json.dumps({"ping":line}))
    except Exception as e:
        print e
        print "Ping Unexpected error:", sys.exc_info()[0]
    p.terminate()

def get_nbr_info(ws):
    global device_manager
    info = {}
    t1 = tlvlib.create_get_tlv64(0, tlvlib.VARIABLE_UNIT_BOOT_TIMER)
    t2 = tlvlib.create_get_tlv128(0, tlvlib.VARIABLE_SW_REVISION)
    t3 = tlvlib.create_get_tlv256(device_manager.brm_instance, 0x101)
    enc, tlvs = tlvlib.send_tlv([t1,t2,t3], "localhost")
    info['BootedAt'] = tlvlib.get_start_ieee64_time_as_string(tlvs[0].int_value)
    info['Uptime'] = tlvlib.convert_ieee64_time_to_string(tlvs[0].int_value)
    info['BorderRouterSW'] = tlvlib.convert_string(tlvs[1].value)
    info['SerialInterface'] = "/dev/" + tlvlib.convert_string(tlvs[2].value)

    t1 = tlvlib.create_get_tlv32(device_manager.radio_instance,
                                 tlvlib.VARIABLE_RADIO_PAN_ID)
    t2 = tlvlib.create_get_tlv32(device_manager.radio_instance,
                                 tlvlib.VARIABLE_RADIO_CHANNEL)
    t3 = tlvlib.create_get_tlv128(device_manager.brm_instance, 0x105)
    enc, tlvs = tlvlib.send_tlv([t1,t2,t3], "localhost")
    info['PanID'] = "0x%04x"%tlvs[0].int_value
    info['Channel'] = tlvs[1].int_value
    info['SerialRadioSW'] = tlvlib.convert_string(tlvs[2].value)
    ws.sendMessage(json.dumps(info))

def _handle_nbrset(ws, cmds):
    name = cmds[1]
    try:
        if name == "channel":
            set_channel(ws, cmds[2])
        elif name == "panid":
            set_panid(ws, cmds[2])
        elif name == "globalrepair":
            do_global_repair(ws)
        else:
            send_error(ws, 'nbrset does not support: ' + name)
    except Exception as e:
        print e
        print "Unexpected error trying to set " + name + ":", sys.exc_info()[0]
        send_error(ws, "Unexpected error trying to set " + name + ": " +  str(e))

def set_channel(ws, channel):
    global device_manager
    channel = int(channel)
    if channel >= 11 and channel <= 26:
        tlv = tlvlib.create_set_tlv(device_manager.radio_instance,
                                    tlvlib.VARIABLE_RADIO_CHANNEL,
                                    tlvlib.SIZE32, struct.pack("!L", channel))
        enc, tlvs = tlvlib.send_tlv([tlv], "localhost")
        print "channel set to:", channel
        send_response(ws, "Channel set to: " + str(channel))

        # Update table
        get_nbr_info(ws)
    else:
        print "Error trying to set channel to:" + str(channel)
        send_error(ws, "Illegal channel: " + str(channel))

def do_global_repair(ws):
    global device_manager
    print "Global Network Repair!"
    tlv = tlvlib.create_set_tlv(device_manager.brm_instance, 0x10a,
                                tlvlib.SIZE32, struct.pack("!L", 1))
    enc, tlvs = tlvlib.send_tlv([tlv], "localhost")
    send_response(ws, "Global Network Repair!")

def set_panid(ws, panid):
    global device_manager
    print "setting pandid", panid
    panid = tlvlib.decodevalue(panid)
    tlv = tlvlib.create_set_tlv(device_manager.radio_instance,
                                tlvlib.VARIABLE_RADIO_PAN_ID,
                                tlvlib.SIZE32, struct.pack("!L", panid))
    enc, tlvs = tlvlib.send_tlv([tlv], "localhost")
    print "panid set to: " + str(panid)
    send_response(ws, "Panid set to: " + str(panid))
    # Update table
    get_nbr_info(ws)

def stop(ws):
    # Stop any activity for this specific web socket
    ws.stop = True

def setup_state():
    global nbr_address
    global device_manager

    nbr_address = device_manager.router_address
    if nbr_address == None:
        print "Did not find the address of the border router"
        sys.exit(1)

    httpd.set_forward_prefix(device_manager.router_prefix)
    httpd.init()

    print "WS Server state initialized."

    # Start a web server
    thread.start_new_thread(httpd.serve_forever, ())

    # Start a device server
    thread.start_new_thread(device_manager.serve_forever, ())
    device_manager.add_event_listener(test_listener)


# plugins that serve data for the websockets
plugins = []

# The demo socket code
class DemoSocket(WebSocket):

    stop = False

    def get_device_manager(self):
        return device_manager

    def get_nbr_address(self):
        return nbr_address

    def handleMessage(self):
        global nbr_address

        if self.data is None:
            self.data = ''

        # if no NBR is found then retry to "connect" to it.
        if nbr_address is None:
            setup_state()
            if nbr_address is None:
                self.sendMessage('{"nbr":"''"}')
                return

        # echo message back to client
        if DEBUG: print "Got data: ", self.data
        cmds = string.split(str(self.data))
        if DEBUG: print "Cmd: ", cmds[0]
        try:
            if cmds[0] == "ping":
                ip = cmds[1]
                thread.start_new_thread(ping, (self,ip))
            elif cmds[0] == "nbrset":
                _handle_nbrset(self, cmds)
            elif cmds[0] == "close":
                self.sendClose()
            elif cmds[0] == "nbr":
                if nbr_address is None:
                    self.sendMessage('{"nbr":"''"}')
                else:
                    self.sendMessage('{"nbr":"' + nbr_address + '"}')
            elif cmds[0] == "channel":
                get_nbr_info(self)
            elif cmds[0] == "stop":
                stop(self)
            else:
                # Check if the commands matches any of the plugins
                for plugin in plugins:
                    if cmds[0] in plugin.get_commands():
                        plugin.handle_command(self, cmds)
        except Exception as e:
            print e
            print "DemoSocket: Unexpected error handling cmd:", cmds[0], sys.exc_info()[0]


    def handleConnected(self):
        global websockets
        print self.address, 'connected'
        if self in websockets:
            print "Warning: already in list???"
        else:
            websockets = websockets + [self]

    def handleClose(self):
        global websockets
        if self in websockets:
            websockets.remove(self)
        print self.address, 'closed - list:', websockets


def test_listener(device_event):
    print "Got Device Event: address:", device_event.device.address, " EType", device_event.event_type
    send = False
    data = ""
    device = device_event.device
    if device_event.event_type is deviceserver.EVENT_DISCOVERY:
        #inform all sockets - will this break if something is removed?
        send = True
        data = json.dumps({'event':{'type':device_event.event_type,
                                    'address':device.address,
                                    'label':device.label
                                }})
    elif device_event.event_type is deviceserver.EVENT_BUTTON:
        send = True
        data = json.dumps({'event':{'type':device_event.event_type,
                                    'address':device.address,
                                    'count':device_event.event_data
                                }})
    # fix this for handling delete/add to the websockets list...
    if send:
        for ws in websockets:
            ws.sendMessage(data)

def usage():
    print "Usage:",sys.argv[0],"[-c channel] [-P panid]"
    exit(0)

if __name__ == "__main__":
    arg = 1

    device_manager = deviceserver.DeviceServer()
    try:
        if not device_manager.setup():
            print "No border router found. Please make sure a border router is running!"
            exit(1)
    except socket.timeout:
        print "No border router found. Please make sure a border router is running!"
        exit(1)
    except Exception as e:
        print e
        print "Failed to connect to border router."
        exit(1)
    while len(sys.argv) > arg + 1:
        if sys.argv[arg] == "-c":
            device_manager.radio_channel = tlvlib.decodevalue(sys.argv[arg + 1])
        elif sys.argv[arg] == "-P":
            device_manager.radio_panid = tlvlib.decodevalue(sys.argv[arg + 1])
        else:
            break
        arg += 2

    if len(sys.argv) > arg:
        if sys.argv[arg] == "-h":
            usage()
    device_manager.set_channel_panid()
    print "Starting demo server"
    setup_state()
    server = SimpleWebSocketServer('', 8001, DemoSocket)
    plugins = plugins + [wspserial.SerialCommands(), wsptlvs.TLVCommands(), wspnodes.NodeCommands()]
    server.serveforever()
