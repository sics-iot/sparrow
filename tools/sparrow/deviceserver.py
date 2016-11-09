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
# Grab devices using Sparrow Application Layer and keep them in the network
#

import tlvlib, nstatslib, sys, struct, binascii, socket, time, threading
import subprocess, re

EVENT_DISCOVERY = "discovery"
EVENT_BUTTON = "button"
EVENT_NSTATS_RPL = "nstats-rpl"

class Device:
    product_type = None
    device_info = None
    label = 'unknown'
    boot_time = 0
    address = ""

    button_instance = None
    leds_instance = None
    temperature_instance = None
    nstats_instance = None
    sleep_instance = None

    nstats_rpl = None

    next_fetch = 0
    fetch_tries = 0

    next_update = 0
    update_tries = 0
    discovery_tries = 0
    _discovery_lock = False

    _outgoing = None
    _outgoing_callbacks = None

    def __init__(self, device_address, device_server):
        self.address = device_address
        self._device_server = device_server
        self.last_seen = 0
        self.last_ping = 0

    def is_discovered(self):
        return self.product_type is not None

    def is_sleepy_device(self):
        return _outgoing is not None

    def set_sleepy_device(self):
        if self._outgoing is None:
            self._outgoing = []

    def get_instance(self, instance_type):
        if self.device_info:
            i = 1
            for data in self.device_info[1]:
                if data[0] == instance_type:
                    return i
                i += 1
        return None

    def send_tlv(self, tlv, callback = None):
        if self._outgoing is None:
            if callback:
                return self._send_immediately(tlv, [callback])
            else:
                return self._send_immediately(tlv)

        if type(tlv) == list:
            self._outgoing += tlv
        else:
            self._outgoing.append(tlv)
        if callback:
            if self._outgoing_callbacks:
                self._outgoing_callbacks.append(callback)
            else:
                self._outgoing_callbacks = [callback]
        return True

    def get_pending_packet_count(self):
        if self._outgoing:
            return len(self._outgoing)
        return 0

    def _flush(self):
        if self._outgoing is not None and len(self._outgoing) > 0:
            print "Flushing",len(self._outgoing),"TLVs to",self.address,"at",time.time()

            tlvs = self._outgoing
            self._outgoing = []

            callbacks = None
            if self._outgoing_callbacks:
                callbacks = self._outgoing_callbacks
                self._outgoing_callbacks = None

            self._send_immediately(tlvs, callbacks)

    def _send_immediately(self, tlv, callbacks = None):
        try:
            enc, tlvs = tlvlib.send_tlv(tlv, self.address)
            self._process_tlvs(tlvs)
            if callbacks:
                for callback in callbacks[:]:
                    try:
                        callback(self, tlvs)
                    except Exception as e:
                        print "*** TLV callback error:", e
            return True
        except socket.timeout:
            print "Failed to send to",self.address,"at",time.time(),"(time out)"
            return False
        except IOError as e:
            print "Failed to send to", self.address, e
            return False

    def _process_tlvs(self, tlvs):
        self.last_seen = time.time();
        for tlv in tlvs:
            if tlv.error != 0:
                print "Received error:"
                tlvlib.print_tlv(tlv)

                if tlv.instance == 0:
                    if tlv.variable == tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG:
                        print "Failed to update WDT for",self.address,"- trying to grab!"
                        self._device_server.grab_device(self.address)

            elif tlv.instance == 0:
                if tlv.variable == tlvlib.VARIABLE_UNIT_BOOT_TIMER:
                    # Update the boot time estimation
                    seconds,ns = tlvlib.convert_ieee64_time(tlv.int_value)
                    last_boot_time = self.boot_time
                    self.boot_time = self.last_seen - seconds
                    # Assume reboot if the boot time backs at least 30 seconds
                    if last_boot_time - self.boot_time > 30:
                        print "Node reboot detected!", self.address
                elif tlv.variable == tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG:
                    print "WDT updated in", self.address
                    self.next_update = self.last_seen + self._device_server.watchdog_time - self._device_server.guard_time
                    self.update_tries = 0

            elif self.nstats_instance and tlv.instance == self.nstats_instance:
                if tlv.variable == tlvlib.VARIABLE_NSTATS_DATA:
                    self._handle_nstats(tlv)

    def _handle_nstats(self, tlv):
        if tlv.error != 0:
            return
        nstats = nstatslib.Nstats(tlv.value)
        if not nstats:
            return
        rpl = nstats.get_data_by_type(nstatslib.NSTATS_TYPE_RPL)
        if not rpl:
            return
        self.nstats_rpl = rpl
        de = DeviceEvent(self, EVENT_NSTATS_RPL, rpl)
        self._device_server.send_event(de)

    def arm_device(self, instances):
        tlvs = [tlvlib.create_set_vector_tlv(0, tlvlib.VARIABLE_EVENT_ARRAY,
                                          0, 0, 1,
                                          struct.pack("!L", 1))]
        if type(instances) == list:
            for i in instances:
                t = tlvlib.create_set_vector_tlv(i, tlvlib.VARIABLE_EVENT_ARRAY,
                                              0, 0, 2,
                                              struct.pack("!LL", 1, 2))
                tlvs.append(t)
        else:
            t = tlvlib.create_set_vector_tlv(instances,
                                          tlvlib.VARIABLE_EVENT_ARRAY, 0, 0, 2,
                                          struct.pack("!LL", 1, 2))
            tlvs.append(t)

        if not self.send_tlv(tlvs):
            print "Failed to arm device"

    def __str__(self):
        return "Device(" + self.address + ")"

# DeviceEvent used for different callbacks
class DeviceEvent:

    def __init__(self, device = None, event_type = None, event_data = None):
        self.device = device
        self.event_type = event_type
        self.event_data = event_data

    def __str__(self):
        addr = None
        if self.device:
            addr = self.device.address
        return "DeviceEvent(" + `addr` + "," + `self.event_type` + "," + `self.event_data` + ")"

class DeviceServer:
    _sock = None

    device_server_host = None
    router_host = "localhost"
    router_address = None
    router_prefix = None
    router_instance = None
    brm_instance = None
    nstats_instance = None
    radio_instance = None
    radio_channel = 26
    radio_panid = 0xabcd

    udp_address = "aaaa::1"
    udp_port = 4444

    # The open range is 7000 - 7999
    # This is what is announced in the beacon
    location = 7000
    # This is what is set when grabbing the node
    grab_location = 0

    # watchdog time in seconds
    watchdog_time = 600
    guard_time = 300

    # Try to grab any device it hears
    grab_all = 0
    _accept_nodes = None

    fetch_time = 60

    def __init__(self):
        self._devices = {}
        self._callbacks = []

    def send_event(self, device_event):
        print "Sending event:", device_event
        for callback in self._callbacks[:]:
#            print "Callback to:", callback
            try:
                callback(device_event)
            except Exception as e:
                print "*** callback error:", e

    def add_event_listener(self, callback):
        self._callbacks.append(callback)

    def remove_event_listener(self, callback):
        self._callbacks.remove(callback)

    # 24-bit reserved, 8-bit type = 02
    # 16-bit reserved, 16-bit port
    # 16-byte IPv6 address
    # 4-byte location ID
    # 4-byte reserved
    def grab_device(self, target):
        IPv6Str = binascii.hexlify(socket.inet_pton(socket.AF_INET6, self.udp_address))
        payloadStr = "000000020000%04x"%self.udp_port + IPv6Str + "%08x"%self.grab_location + "00000000"
        payload = binascii.unhexlify(payloadStr)
        print "Grabbing device", target, " => ", payloadStr, len(payload)
        t1 = tlvlib.create_set_tlv(0, tlvlib.VARIABLE_UNIT_CONTROLLER_ADDRESS, 3,
                                 payload)
        t2 = tlvlib.create_set_tlv32(0, tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG,
                                     self.watchdog_time)
        if hasattr(target, 'send_tlv'):
            if not target.send_tlv([t1,t2]):
                print "Failed to grab device (time out)"
                return False
            return True

        try:
            tlvlib.send_tlv([t1,t2], target)
            return True
        except socket.timeout:
            print "Failed to grab device (time out)"
            return False

    def set_location(self, address, location):
        print "Setting location on", address, " => ", location
        t = tlvlib.create_set_tlv32(0, tlvlib.VARIABLE_LOCATION_ID, location)
        enc,tlvs = tlvlib.send_tlv(t, address)
        return tlvs

    def discover_device(self, dev):
        if dev._discovery_lock:
            return
        dev._discovery_lock = True
        dev.discovery_tries = dev.discovery_tries + 1
        try:
            print "Trying to TLV discover device:", dev.address
            dev.device_info = tlvlib.discovery(dev.address)
            dev.product_type = dev.device_info[0][1]
            dev.label = dev.device_info[0][0]
            print "\tFound: ", dev.device_info[0][0], " Product Type: 0x%016x"%dev.product_type
            seconds,nanoseconds = tlvlib.convert_ieee64_time(dev.device_info[0][2])
            dev.boot_time = time.time() - seconds

            i = 1
            for data in dev.device_info[1]:
                if data[0] == tlvlib.INSTANCE_BUTTON_GENERIC:
                    dev.button_instance = i
                elif data[0] == tlvlib.INSTANCE_LEDS_GENERIC:
                    dev.leds_instance = i
                elif data[0] == tlvlib.INSTANCE_TEMP_GENERIC:
                    dev.temperature_instance = i
                elif data[0] == tlvlib.INSTANCE_NETWORK_STATISTICS:
                    print "\tFound:  Network Statistics"
                    dev.nstats_instance = i
                elif data[0] == tlvlib.INSTANCE_SLEEP:
                    print "\tFound:  Sleep instance"
                    dev.sleep_instance = i
                    dev.set_sleepy_device()
                i += 1

            if dev.next_update == 0:
               if self.grab_device(dev.address):
                    dev.next_update = time.time() + self.watchdog_time - self.guard_time
            if dev.button_instance:
                print "\tFound:  Button instance - arming device!"
                dev.arm_device(dev.button_instance)

            de = DeviceEvent(dev, EVENT_DISCOVERY)
            self.send_event(de)
        except Exception as e:
            print e
        dev._discovery_lock = False

    def ping(self, dev):
        print "Pinging: ", dev.address, " to check liveness..."
        p=subprocess.Popen(["ping6", "-c", "1", "-W", "2", dev.address],
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            while True:
                line = p.stdout.readline()
                if line == '': break
                print line
        except Exception as e:
            print e
            print "Ping Unexpected error:", sys.exc_info()[0]
        p.wait()
        dev.last_ping = time.time()
        if p.returncode == 0:
            dev.last_seen = dev.last_ping
        if p.returncode == None:
            p.terminate()

    # ----------------------------------------------------------------
    # manage devices
    # this will maintain the devices - keep the "grabbed" by updating the
    # watchdog timer - Note: might need to keep a add/delete list to aovid
    # messing with the same lists when devices need to be added/deleted.
    # ----------------------------------------------------------------
    def _manage_devices(self):
        while True:
            current_time = time.time()
            remove_list = []
            for dev in self.get_devices():
                # Check if there is need for discovery
                if not dev.is_discovered():
                    if dev.discovery_tries < 5:
                        self.discover_device(dev)

                    if not dev.is_discovered():
                        # Do a live check if needed...
                        if dev.last_seen + 60 < current_time and dev.last_ping + 60 < current_time:
                            self.ping(dev)

                        # Remove non-discoverable devices after some time
                        # so they can be tried again
                        if dev.last_seen + 180 < current_time:
                            print "Removing",dev.address,"last seen",(current_time - dev.last_seen),"seconds ago"
                            remove_list.append(dev)
                        continue

                # Check if there is need for WDT update
                if current_time > dev.next_update:
                    print "Updating WDT in", dev.address
                    dev.update_tries += 1
                    if dev.update_tries > 20:
                        print "Removing",dev.address,"due to too",dev.update_tries,"WDT update retries"
                        remove_list.append(dev)
                    else:
                        t1 = tlvlib.create_set_tlv32(0, tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG, self.watchdog_time)
                        try:
                            # Retry every minute
                            dev.next_update = current_time + 60
                            dev.send_tlv(t1)
                        except Exception as e:
                            print "Failed to update WDT in", dev.address
                            print e

                if current_time >= dev.next_fetch:
                    dev.fetch_tries += 1
                    if self._fetch_periodic(dev):
                        dev.fetch_tries = 0
                        dev.next_fetch = time.time() + self.fetch_time
                    else:
                        print "*** failed to fetch from", dev.address,"("+str(dev.fetch_tries)+")"
                        # Try again a little later
                        dev.next_fetch = time.time() + 10 * dev.fetch_tries

            for dev in remove_list:
                print "Removing device", dev.address
                self.remove_device(dev.address)

            time.sleep(1)


    def get_devices(self):
        return list(self._devices.values())

    def get_device(self, addr):
        if addr in self._devices:
            return self._devices[addr]
        return None

    # adds a device to the grabbed devices list
    def add_device(self, addr):
        d = self.get_device(addr)
        if d is not None:
            return d;
        print "Adding device",addr

        d = Device(addr, self)
        d.last_seen = time.time()
        # to avoid pinging immediately
        d.last_ping = d.last_seen

        self._devices[addr] = d
        return d

    def remove_device(self, addr):
        del self._devices[addr]

    def fetch_nstats(self):
        t = threading.Thread(target=self._fetch_nstats)
        t.daemon = True
        t.start()

    def _fetch_nstats(self):
        for dev in self.get_devices():
            if dev.nstats_instance:
                # The device has the network instance
                print "Fetching network statistics from", dev.address
                try:
                    t = nstatslib.create_nstats_tlv(dev.nstats_instance)
                    if not dev.send_tlv(t):
                        print "*** Failed to fetch network statistics from", dev.address
                except Exception as e:
                    print "**** Failed to fetch network statistics:", e
                time.sleep(0.5)

    def _lookup_device_host(self, prefix, default_host):
        try:
            output = subprocess.check_output('ifconfig | grep inet6 | grep -v " fe80" | grep -v " ::1"', shell=True)
            # Check prefix first
            p = re.compile(" (" + prefix + "[a-fA-F0-9:]+)(/| prefixlen )")
            m = p.search(output)
            if m:
                return m.group(1)

            p = re.compile(" ([a-fA-F0-9:]+)(/| prefixlen )")
            m = p.search(output)
            if m:
                return m.group(1)
            else:
                print "----------"
                print "ERROR: Failed to lookup device host address:"
                print output
                print "----------"
        except Exception as e:
            print "----------"
            print e
            print "ERROR: Failed to lookup device host address:", sys.exc_info()[0]
            print "----------"
            return default_host

    def is_device_acceptable(self, host, device_type = 0):
        if self._accept_nodes is None:
            return True
        if host.endswith(self._accept_nodes):
            return True
        return False

    def _fetch_periodic(self, device):
        t = []
        t.append(tlvlib.create_get_tlv64(0, tlvlib.VARIABLE_UNIT_BOOT_TIMER))
        if device.nstats_instance is not None:
            t.append(nstatslib.create_nstats_tlv(device.nstats_instance))

        if device.button_instance is not None:
            t.append(tlvlib.create_set_vector_tlv(0, tlvlib.VARIABLE_EVENT_ARRAY,
                                               0, 0, 1,
                                               struct.pack("!L", 1)))
            t.append(tlvlib.create_set_vector_tlv(device.button_instance,
                                               tlvlib.VARIABLE_EVENT_ARRAY,
                                               0, 0, 2,
                                               struct.pack("!LL", 1, 2)))

        try:
            print "Fetching periodic data from", device.address
            return device.send_tlv(t)
        except socket.timeout:
            print "Failed to fetch from device (time out)"
            return False

    def setup(self):
        # discover and - if it is a NBR then find serial radio and configure the
        # beacons to something good!
        d = tlvlib.discovery(self.router_host)
        print "Product label:", d[0][0]

        if d[0][1] != tlvlib.INSTANCE_BORDER_ROUTER:
            print "Error - could not find the radio - not starting the device server"
            return False

        i = 1
        for data in d[1]:
            print "instance:",i , " type: %016x"%data[0], " ", data[1]
            # check for radio and if found - configure beacons
            if data[0] == tlvlib.INSTANCE_RADIO:
                self.radio_instance = i
            elif data[0] == tlvlib.INSTANCE_ROUTER:
                self.router_instance = i
                t = tlvlib.create_get_tlv(i, tlvlib.VARIABLE_NETWORK_ADDRESS, 2)
                enc, t = tlvlib.send_tlv(t, self.router_host)

                self.router_address = socket.inet_ntop(socket.AF_INET6, t[0].value)
                print "\tRouter address:", self.router_address

                self.router_prefix = socket.inet_ntop(socket.AF_INET6, t[0].value[0:8] + binascii.unhexlify("0000000000000000"))
                #while self.router_prefix.endswith("::"):
                #    self.router_prefix = self.router_prefix[0:len(self.router_prefix) - 1]
                print "\tNetwork prefix:",self.router_prefix
                if self.device_server_host:
                    self.udp_address = self.device_server_host
                else:
                    self.udp_address = self._lookup_device_host(self.router_prefix, socket.inet_ntop(socket.AF_INET6, t[0].value[0:8] + binascii.unhexlify("0000000000000001")))
                print "\tNetwork address:",self.udp_address
            elif data[0] == tlvlib.INSTANCE_BORDER_ROUTER_MANAGEMENT:
                self.brm_instance = i
            elif data[0] == tlvlib.INSTANCE_NETWORK_STATISTICS:
                self.nstats_instance = i
            i = i + 1

        if not self.radio_instance:
            print "Error - could not find the radio instance - not starting the device server"
            return False

        # Setup socket to make sure it is possible to bind the address
        try:
            self._sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._sock.bind((self.udp_address, self.udp_port))
            #self._sock.bind(('', self.udp_port))
        except Exception as e:
            print e
            print "Error - could not bind to the address", self.udp_address
            return False

        # set-up beacon to ...
        IPv6Str = binascii.hexlify(socket.inet_pton(socket.AF_INET6, self.udp_address))
        BEACON = "fe02010a020090da01%08x"%self.location + "18020090da03" + IPv6Str + "%4x"%self.udp_port + "000000"
        beacon_payload = binascii.unhexlify(BEACON)
        print "Setting beacon with length",len(beacon_payload),"in instance", self.radio_instance
        print "\t",BEACON
        t = tlvlib.create_set_vector_tlv(self.radio_instance,
                                      tlvlib.VARIABLE_RADIO_BEACON_RESPONSE,
                                      0, 0, len(beacon_payload) / 4,
                                      beacon_payload)
        enc, t = tlvlib.send_tlv(t, self.router_host)
#        print "Result:"
#        tlvlib.print_tlv(t[0])
        return True

    def set_channel_panid(self):
        # set radio channel
        t1 = tlvlib.create_set_tlv32(self.radio_instance,
                                     tlvlib.VARIABLE_RADIO_CHANNEL,
                                     self.radio_channel)
        # set radio PAN ID
        t2 = tlvlib.create_set_tlv32(self.radio_instance,
                                     tlvlib.VARIABLE_RADIO_PAN_ID,
                                     self.radio_panid)
        tlvlib.send_tlv([t1,t2], self.router_host)

    def serve_forever(self):
        # Initialize if not already initialized
        if not self.router_instance:
            if not self.setup():
                # Initialization failed
                return False

        print "Device server started at [" + self.udp_address + "]:" + str(self.udp_port)

        # do device management each 10 seconds
        t = threading.Thread(target=self._manage_devices)
        t.daemon = True
        t.start()
        while True:
            try:
                data, addr = self._sock.recvfrom(1024)
                host, port, _, _ = addr
                request_temp = False
                print "Received from ", addr, binascii.hexlify(data)
                enc = tlvlib.parse_encap(data)
                tlvs = tlvlib.parse_tlvs(data[enc.size():])
                device = self.get_device(host)
                if device is not None:
                    device.last_seen = time.time();
                dev_type = 0
                button_counter = None
#                print " Received TLVs:"
#                tlvlib.print_tlvs(tlvs)
                for tlv in tlvs:
                    if tlv.error != 0:
                        print "Received error:"
                        tlvlib.print_tlv(tlv)
                    elif tlv.instance == 0:
                        if tlv.variable == 0:
                            dev_type = tlv.int_value
                        elif tlv.variable == tlvlib.VARIABLE_UNIT_BOOT_TIMER:
                            if device:
                                # Update the boot time estimation
                                seconds,nanoseconds = tlvlib.convert_ieee64_time(tlv.int_value)
                                device.boot_time = time.time() - seconds
                                request_temp = True
                                print host,"booted",seconds,"seconds ago"
                        elif tlv.variable == tlvlib.VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX:
                            pass
                        elif tlv.variable == tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG:
                            if not device and not self.is_device_acceptable(host, dev_type):
                                print "IGNORING node", host, "of type 0x%016x"%dev_type, "that could be taken over"
                            else:
                                print "FOUND device",host,"of type 0x%016x"%dev_type,"that can be taken over - WDT = 0"
                                if self.grab_device(host):
                                    if not device:
                                        device = self.add_device(host)
                                    device.next_update = time.time() + self.watchdog_time - self.guard_time
                    elif device and device.button_instance and tlv.instance == device.button_instance:
                        if tlv.variable == tlvlib.VARIABLE_GPIO_TRIGGER_COUNTER:
                            button_counter = tlv.int_value
                        elif tlv.variable == tlvlib.VARIABLE_EVENT_ARRAY:
                            if button_counter is not None:
                                print "Button pressed at node",host,"-",button_counter,"times"
                            # Rearm the button
                            device.arm_device(device.button_instance)

                            if button_counter is not None:
                                de = DeviceEvent(device, EVENT_BUTTON, button_counter)
                                self.send_event(de)
                    elif device and device.nstats_instance and tlv.instance == device.nstats_instance:
                        if tlv.variable == tlvlib.VARIABLE_NSTATS_DATA:
                            device._handle_nstats(tlv)
                if not device:
                    if self.grab_all > 0 and self.is_device_acceptable(host, dev_type):
                        # Unknown device - do a grab attempt
#                        time.sleep(0.005)
                        if self.grab_device(host):
                            device = self.add_device(host)
                            device.next_update = time.time() + self.watchdog_time - self.guard_time
                            self.discover_device(device)
                elif device.is_discovered():
#                    time.sleep(0.005)
                    if request_temp and device.get_pending_packet_count() == 0:
                        t = tlvlib.create_get_tlv64(0, tlvlib.VARIABLE_UNIT_BOOT_TIMER)
                        device.send_tlv(t)

                    device._flush()
                else:
                    self.discover_device(device)

            except socket.timeout:
                print "."
                pass

def usage():
    print "Usage:",sys.argv[0],"[-b bind-address] [-a host] [-c channel] [-P panid] [-t node-address-list] [device]"
    exit(0)

if __name__ == "__main__":
    # stuff only to run when not called via 'import' here
    manage_device = None
    arg = 1

    server = DeviceServer()

    while len(sys.argv) > arg + 1:
        if sys.argv[arg] == "-a":
            server.router_host = sys.argv[arg + 1]
        elif sys.argv[arg] == "-c":
            server.radio_channel = tlvlib.decodevalue(sys.argv[arg + 1])
        elif sys.argv[arg] == "-P":
            server.radio_panid = tlvlib.decodevalue(sys.argv[arg + 1])
        elif sys.argv[arg] == "-b":
            server.device_server_host = sys.argv[arg + 1]
        elif sys.argv[arg] == "-g":
            server.grab_all = tlvlib.decodevalue(sys.argv[arg + 1])
        elif sys.argv[arg] == "-t":
            server._accept_nodes = tuple(sys.argv[arg + 1].split(','))
        else:
            break
        arg += 2

    if len(sys.argv) > arg:
        if sys.argv[arg] == "-h":
            usage()
        manage_device = sys.argv[arg]
        arg += 1

    if len(sys.argv) > arg:
        print "Too many arguments"
        exit(1)

    if manage_device:
        server.add_device(manage_device)

    try:
        if not server.setup():
            print "No border router found. Please make sure a border router is running!"
            sys.exit(1)
    except socket.timeout:
        print "No border router found. Please make sure a border router is running!"
        sys.exit(1)
    except Exception as e:
        print e
        print "Failed to connect to border router."
        sys.exit(1)

    server.set_channel_panid()
    server.serve_forever()
    print "*** Error - device server stopped"
