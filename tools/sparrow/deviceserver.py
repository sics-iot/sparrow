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
import subprocess, re, dscli, logging

EVENT_DISCOVERY = "discovery"
EVENT_BUTTON = "button"
EVENT_MOTION = "motion"
EVENT_NSTATS_RPL = "nstats-rpl"

class EndPoint:
    sock = None
    host = None
    port = None

    def __init__(self, sock, host, port=tlvlib.UDP_PORT):
        self.sock = sock
        self.host = host
        self.port = port

    def sendto(self, data):
        self.sock.sendto(data, (self.host, self.port))

    def __str__(self):
        return "EndPoint(" + `self.sock` + "," + `self.host` + "," + `self.port` + ")"

class Device:
    endpoint = None
    product_type = None
    device_info = None
    label = 'unknown'
    boot_time = 0
    address = ""
    log = None

    button_instance = None
    button_counter = None
    leds_instance = None
    temperature_instance = None
    nstats_instance = None
    sleep_instance = None
    motion_instance = None
    motion_counter = None

    nstats_rpl = None

    next_fetch = 0
    fetch_tries = 0

    next_update = 0
    update_tries = 0
    discovery_tries = 0
    _discovery_lock = False

    _outgoing = None
    _outgoing_callbacks = None
    _pending_outgoing = False

    def __init__(self, device_address, device_endpoint, device_server):
        self.endpoint = device_endpoint
        self.address = device_address
        self._device_server = device_server
        self.last_seen = 0
        self.last_ping = 0
        self.log = logging.getLogger(device_address)

    def is_discovered(self):
        return self.product_type is not None

    def is_sleepy_device(self):
        return self._outgoing is not None

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

    def has_pending_tlv(self, tlv):
        if self._outgoing is None:
            return False
        if type(tlv) == list:
            for t in tlv:
                if self.has_pending_tlv(t):
                    return True
        else:
            for t in self._outgoing:
                if t.instance == tlv.instance and t.variable == tlv.variable and t.op == tlv.op and t.element_size == tlv.element_size and t.length == tlv.length:
                    return True
        return False

    def send_tlv(self, tlv, callback = None):
        if callback:
            if self._outgoing_callbacks:
                self._outgoing_callbacks.append(callback)
            else:
                self._outgoing_callbacks = [callback]
        if self._outgoing is None:
            return self._send_immediately(tlv)

        if type(tlv) == list:
            self._outgoing += tlv
        else:
            self._outgoing.append(tlv)
        return True

    def get_pending_packet_count(self):
        if self._outgoing:
            return len(self._outgoing)
        return 0

    def _flush(self):
        if self._outgoing is not None and len(self._outgoing) > 0:
            self.log.debug("FLUSH %s",tlvlib.get_tlv_short_info(self._outgoing))

            tlvs = self._outgoing
            self._outgoing = []

            self._send_immediately(tlvs)

    def _send_immediately(self, tlv):
        self._pending_outgoing = True
        data = tlvlib.create_encap(tlv)
        self.endpoint.sendto(data)
        return True

    def _process_tlvs(self, tlvs):
        self.last_seen = time.time();
        for tlv in tlvs:
            if tlv.error != 0:
                # TLV errors has already been printed when received

                if tlv.instance == 0:
                    if tlv.variable == tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG:
                        self.log.warning("Failed to update WDT - trying to grab!")
                        self._device_server.grab_device(self)
            elif tlv.op & 0x7f == tlvlib.TLV_GET_RESPONSE:
                self._process_get_response(tlv)
            elif tlv.op & 0x7f == tlvlib.TLV_EVENT_RESPONSE:
                self._process_get_response(tlv)

            # TODO handle other types
        if self._pending_outgoing:
            self._pending_outgoing = False
            if self._outgoing_callbacks:
                callbacks = self._outgoing_callbacks
                self._outgoing_callbacks = None
                for callback in callbacks[:]:
                    try:
                        callback(self, tlvs)
                    except Exception as e:
                        self.log.error("*** TLV callback error: %s", str(e))


    def _process_get_response(self, tlv):
        if tlv.instance == 0:
            if tlv.variable == tlvlib.VARIABLE_UNIT_BOOT_TIMER:
                # Update the boot time estimation
                seconds,ns = tlvlib.convert_ieee64_time(tlv.int_value)
                last_boot_time = self.boot_time
                self.boot_time = self.last_seen - seconds
                # Assume reboot if the boot time backs at least 30 seconds
                if last_boot_time - self.boot_time > 30:
                    self.log.info("REBOOT DETECTED!")
            elif tlv.variable == tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG:
                if self.next_update < 0:
                    # Ignore watchdog in this device
                    pass
                elif hasattr(tlv, 'int_value') and tlv.int_value > 0:
                    self.log.debug("WDT updated! %d seconds remaining",
                                   tlv.int_value)
                    self.next_update = self.last_seen + tlv.int_value - self._device_server.guard_time
                    self.update_tries = 0
                else:
                    # Need to refresh watchdog
                    t1 = tlvlib.create_set_tlv32(0, tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG,
                                                 self._device_server.watchdog_time)
                    self.send_tlv(t1)

            elif tlv.variable == tlvlib.VARIABLE_SLEEP_DEFAULT_AWAKE_TIME:
                # This node is a sleepy node
                self.set_sleepy_device()

        elif self.button_instance and tlv.instance == self.button_instance:
            if tlv.variable == tlvlib.VARIABLE_GPIO_TRIGGER_COUNTER:
                self.button_counter = tlv.int_value
            elif tlv.variable == tlvlib.VARIABLE_EVENT_ARRAY and tlv.op == tlvlib.TLV_EVENT_RESPONSE:
                self.log.info("button pressed - %d times",self.button_counter)
                # Rearm the button
                self.arm_device(self.button_instance)

                de = DeviceEvent(self, EVENT_BUTTON, self.button_counter)
                self._device_server.send_event(de)
        elif self.motion_instance and tlv.instance == self.motion_instance:
            if tlv.variable == 0x100:
                self.motion_counter, = struct.unpack("!q", tlv.data[8:16])
            elif tlv.variable == tlvlib.VARIABLE_EVENT_ARRAY and tlv.op == tlvlib.TLV_EVENT_RESPONSE:
                self.log.info("MOTION! - %d times",self.motion_counter)
                # Rearm the button
                self.arm_device(self.motion_instance)

                de = DeviceEvent(self, EVENT_MOTION, self.motion_counter)
                self._device_server.send_event(de)
        elif self.nstats_instance and tlv.instance == self.nstats_instance:
            if tlv.variable == tlvlib.VARIABLE_NSTATS_DATA:
                self._handle_nstats(tlv)
        elif self.temperature_instance and tlv.instance == self.temperature_instance:
            if tlv.variable == tlvlib.VARIABLE_TEMPERATURE:
                temperature = (tlv.int_value - 273150) / 1000.0
                self.log.info("Temperature: " + str(round(temperature, 2)) + " C")

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
            log.warning("Failed to arm device!")

    def info(self):
        info = "  " + self.address + "\t"
        if self.is_discovered():
            info += "0x%016x"%self.product_type
        if self.is_sleepy_device():
            info += "\t[sleepy]"
        return info

    def __str__(self):
        return "Device(" + self.address + " , " + str(self.is_sleepy_device()) + ")"

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
    _sock4 = None

    running = True
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
    watchdog_time = 1200
    guard_time = 300

    # Try to grab any device it hears
    grab_all = 0
    _accept_nodes = None

    fetch_time = 120

    def __init__(self):
        self._devices = {}
        self._callbacks = []
        self.log = logging.getLogger("server")

    def send_event(self, device_event):
#        print "Sending event:", device_event
        for callback in self._callbacks[:]:
#            print "Callback to:", callback
            try:
                callback(device_event)
            except Exception as e:
                self.error("*** callback error: %s", str(e))

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
        # Do not try to grab targets that should not be grabbed
        if hasattr(target, 'next_update') and target.next_update < 0:
            return False
        IPv6Str = binascii.hexlify(socket.inet_pton(socket.AF_INET6, self.udp_address))
        payloadStr = "000000020000%04x"%self.udp_port + IPv6Str + "%08x"%self.grab_location + "00000000"
        payload = binascii.unhexlify(payloadStr)
        self.log.info("[%s] Grabbing device => %s (%s)", target, payloadStr, str(len(payload)))
        t1 = tlvlib.create_set_tlv(0, tlvlib.VARIABLE_UNIT_CONTROLLER_ADDRESS, 3,
                                 payload)
        t2 = tlvlib.create_set_tlv32(0, tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG,
                                     self.watchdog_time)
        if hasattr(target, 'send_tlv'):
            if target.has_pending_tlv(t2):
                self.log.info("[%s] Already has pending grab request", target.address)
                return False
            if not target.send_tlv([t1,t2]):
                self.log.info("[%s] Failed to grab device (time out)", target.address)
                return False
            return True

        try:
            tlvlib.send_tlv([t1,t2], target)
            return True
        except socket.timeout:
            self.log.warning("[%s] Failed to grab device (time out)", str(target))
            return False

    def set_location(self, address, location):
        self.log.debug("[%s] Setting location to %d", location)
        t = tlvlib.create_set_tlv32(0, tlvlib.VARIABLE_LOCATION_ID, location)
        enc,tlvs = tlvlib.send_tlv(t, address)
        return tlvs

    def discover_device(self, dev):
        if dev._discovery_lock:
            return
        dev._discovery_lock = True
        dev.discovery_tries = dev.discovery_tries + 1
        try:
            dev.log.debug("trying to do TLV discover")
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
                elif data[0] == tlvlib.INSTANCE_MOTION_GENERIC:
                    dev.motion_instance = i
                elif data[0] == tlvlib.INSTANCE_LEDS_GENERIC:
                    dev.leds_instance = i
                elif data[0] == tlvlib.INSTANCE_TEMP_GENERIC:
                    dev.temperature_instance = i
                elif data[0] == tlvlib.INSTANCE_TEMPHUM_GENERIC:
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
               if self.grab_device(dev):
                    dev.next_update = time.time() + self.watchdog_time - self.guard_time
            if dev.button_instance:
                print "\tFound:  Button instance - arming device!"
                dev.arm_device(dev.button_instance)

            if dev.motion_instance:
                print "\tFound:  Motion instance - arming device!"
                dev.arm_device(dev.motion_instance)

            de = DeviceEvent(dev, EVENT_DISCOVERY)
            self.send_event(de)
        except Exception as e:
            dev.log.error("discovery failed: %s", str(e))
        dev._discovery_lock = False

    def ping(self, dev):
        dev.log.debug("Pinging to check liveness...")
        p = subprocess.Popen(["ping6", "-c", "1", dev.address],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            while True:
                line = p.stdout.readline()
                if line == '': break
                # print line
        except Exception as e:
            print e
            dev.log.error("Ping Unexpected error: %s", str(sys.exc_info()[0]))
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
        while self.running:
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
                if current_time > dev.next_update and dev.next_update >= 0:
                    dev.log.debug("UPDATING WDT!")
                    dev.update_tries += 1
                    if dev.update_tries > 20:
                        print "[" + dev.address + "] REMOVED due to",dev.update_tries,"WDT update retries"
                        remove_list.append(dev)
                    else:
                        t1 = tlvlib.create_set_tlv32(0, tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG, self.watchdog_time)
                        try:
                            # Retry every minute
                            dev.next_update = current_time + 60
                            dev.send_tlv(t1)
                        except Exception as e:
                            print "[" + dev.address + "] FAILED TO UPDATE WDT!"
                            print e

                if current_time >= dev.next_fetch and dev.next_fetch >= 0:
                    dev.fetch_tries += 1
                    if self._fetch_periodic(dev):
                        dev.fetch_tries = 0
                        dev.next_fetch = time.time() + self.fetch_time
                    else:
                        print "*** failed to fetch from", dev.address,"("+str(dev.fetch_tries)+")"
                        # Try again a little later
                        dev.next_fetch = time.time() + 10 * dev.fetch_tries

            for dev in remove_list:
                self.remove_device(dev.address)

            time.sleep(1)


    def get_devices(self):
        return list(self._devices.values())

    def get_device(self, addr):
        if addr in self._devices:
            return self._devices[addr]
        return None

    # adds a device to the grabbed devices list
    def _add_device(self, sock, addr, port=tlvlib.UDP_PORT):
        d = self.get_device(addr)
        if d is not None:
            return d;

        endpoint = EndPoint(sock, addr, port)
        d = Device(addr, endpoint, self)
        d.last_seen = time.time()
        # to avoid pinging immediately
        d.last_ping = d.last_seen
        d.next_fetch = d.last_seen + self.fetch_time
        d.log.debug("ADDED")

        self._devices[addr] = d
        return d

    def add_device(self, addr, port=tlvlib.UDP_PORT):
        self._add_device(self._sock, addr, port)

    def remove_device(self, addr):
        d = self.get_device(addr)
        if d is not None:
            del self._devices[addr]
            d.log.debug("REMOVED")

    def fetch_nstats(self):
        t = threading.Thread(target=self._fetch_nstats)
        t.daemon = True
        t.start()

    def _fetch_nstats(self):
        for dev in self.get_devices():
            if dev.nstats_instance:
                # The device has the network instance
                dev.log.debug("Requesting network statistics")
                try:
                    t = nstatslib.create_nstats_tlv(dev.nstats_instance)
                    if not dev.send_tlv(t):
                        dev.log.debug("*** Failed to fetch network statistics")
                except Exception as e:
                    dev.log.debug("**** Failed to fetch network statistics: %s", str(e))
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
        if False and device.nstats_instance is not None:
            t.append(nstatslib.create_nstats_tlv(device.nstats_instance))

        if device.button_instance is not None:
            t.append(tlvlib.create_set_vector_tlv(0, tlvlib.VARIABLE_EVENT_ARRAY,
                                               0, 0, 1,
                                               struct.pack("!L", 1)))
            t.append(tlvlib.create_set_vector_tlv(device.button_instance,
                                               tlvlib.VARIABLE_EVENT_ARRAY,
                                               0, 0, 2,
                                               struct.pack("!LL", 1, 2)))

        if device.motion_instance is not None:
            if device.button_instance is None:
                t.append(tlvlib.create_set_vector_tlv(0, tlvlib.VARIABLE_EVENT_ARRAY,
                                                      0, 0, 1,
                                                      struct.pack("!L", 1)))
            t.append(tlvlib.create_set_vector_tlv(device.motion_instance,
                                               tlvlib.VARIABLE_EVENT_ARRAY,
                                               0, 0, 2,
                                               struct.pack("!LL", 1, 2)))

        try:
            device.log.debug("requesting periodic data: %s",tlvlib.get_tlv_short_info(t))
            return device.send_tlv(t)
        except socket.timeout:
            device.log.error("failed to fetch from device (time out)")
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
            print "Instance:",i , " type: %016x"%data[0], " ", data[1]
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

        if not self.router_instance:
            print "Error - could not find the router instance - not starting the device server"
            return False

        # Setup socket to make sure it is possible to bind the address
        try:
            self._sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._sock.bind((self.udp_address, self.udp_port))
            #self._sock.bind(('', self.udp_port))
            self._sock.settimeout(1.0)


            self._sock4 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            self._sock4.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._sock4.bind(('localhost', self.udp_port))
            self._sock4.settimeout(1.0)

        except Exception as e:
            print e
            print "Error - could not bind to the address", self.udp_address
            return False

        # set radio channel and panid
        self.set_channel_panid()

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

    def stop(self):
        self.running = False
#        sys.exit(0)

    def wakeup(self, host, time=60):
        device = self.get_device(host)
        if device is not None:
            self._wakeup_device(device, time)
        elif host == "all" or host == "*":
            for dev in self.get_devices():
                if dev.is_sleepy_device():
                    self._wakeup_device(dev, time)
        else:
            print "could not find device with address",host

    def _wakeup_device(self, device, time):
        if device.is_sleepy_device() and device.sleep_instance is not None:
            print "requesting [" + device.address + "] to wakeup"
            t = tlvlib.create_set_tlv32(device.sleep_instance,
                                        tlvlib.VARIABLE_SLEEP_AWAKE_TIME_WHEN_NO_ACTIVITY,
                                        time * 1000L)
            device.send_tlv(t)
        else:
            print device.address,"is not a sleepy device"

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

    def _udp_receive(self, sock):
        print "UDP Receive - on socket: ", sock
        while self.running:
            try:
                data, addr = sock.recvfrom(1024)
                if not self.running:
                    return
#                print "Received from ", addr, binascii.hexlify(data)
                if len(addr) > 2:
                    host, port, _, _ = addr
                else:
                    host, port = addr

                device = self.get_device(host)
                enc = tlvlib.parse_encap(data)
                if enc.error != 0:
                    self.log.error("[%s] RECV ENCAP ERROR %s", '', str(enc.error))
                elif enc.payload_type == tlvlib.ENC_PAYLOAD_TLV:
                    self._process_incoming_tlvs(sock, host, port, device, enc, data[enc.size():])
                else:
                    self.log.error("[%s] RECV ENCAP UNHANDLED PAYLOAD %s", str(enc.payload_type))

            except socket.timeout:
                pass

    def serve_forever(self):
        # Initialize if not already initialized
        if not self.router_instance:
            if not self.setup():
                # Initialization failed
                return False

        self.log.info("Device server started at [%s]:%d", self.udp_address, self.udp_port)

        self.running = True

        # do device management periodically
        t1 = threading.Thread(target=self._manage_devices)
        t1.daemon = True
        t1.start()

        # sock4 socket
        t2 = threading.Thread(target=self._udp_receive, args=[self._sock4])
        t2.daemon = True
        t2.start()
        self._udp_receive(self._sock)
        t2.join()
        t1.join()
        self._sock.close()
        self._sock4.close()
        exit(0)

    def _process_incoming_tlvs(self, sock, host, port, device, enc, data):
        tlvs = tlvlib.parse_tlvs(data)

        if device is not None:
            last_seeen = device.last_seen
            device.last_seen = time.time();
            device.log.debug("RECV %s",tlvlib.get_tlv_short_info(tlvs))
        else:
            last_seen = time.time()
            self.log.debug("[%s] RECV %s",host,tlvlib.get_tlv_short_info(tlvs))

        ping_device = False
        dev_watchdog = None
        dev_type = 0
#                print " Received TLVs:"
#                tlvlib.print_tlvs(tlvs)
        for tlv in tlvs:
            if tlv.error != 0:
                if device is not None:
                    device.log.error("Received error (" + str(tlv.error) + "):")
                else:
                    self.log.error("[%s] Received error:", host)
                tlvlib.print_tlv(tlv)
            elif tlv.instance == 0 and tlv.op == tlvlib.TLV_GET_RESPONSE:
                if tlv.variable == tlvlib.VARIABLE_OBJECT_TYPE:
                    dev_type = tlv.int_value
                elif tlv.variable == tlvlib.VARIABLE_UNIT_BOOT_TIMER:
                    pass
                elif tlv.variable == tlvlib.VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX:
                    if device is not None and last_seen - time.time() > 55:
                        ping_device = True
                elif tlv.variable == tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG:
                    dev_watchdog = tlv.int_value

        if not device:
            if dev_watchdog is not None or self.grab_all == 0:
                if dev_type == tlvlib.INSTANCE_BORDER_ROUTER:
                    # Do not try to grab the border router
                    pass
                elif not self.is_device_acceptable(host, dev_type):
                    self.log.debug("[%s] IGNORING device of type 0x%016x that could be taken over", host, dev_type)
                else:
                    # Unknown device - do a grab attempt
                    if dev_watchdog is not None:
                        self.log.debug("[%s] FOUND new device of type 0x%016x that can be taken over - WDT = %d", host, dev_type, dev_watchdog)

                    if self.grab_device(host):
                        device = self._add_device(sock, host, port)
                        device.next_update = time.time() + self.watchdog_time - self.guard_time
                        self.discover_device(device)
        elif device.is_discovered():
#                    time.sleep(0.005)
            device._process_tlvs(tlvs)
            if not device.is_sleepy_device() and ping_device and device.get_pending_packet_count() == 0:
                t = tlvlib.create_get_tlv64(0, tlvlib.VARIABLE_UNIT_BOOT_TIMER)
                device.send_tlv(t)

            device._flush()
        else:
            self.discover_device(device)

def usage():
    print "Usage:",sys.argv[0],"[-b bind-address] [-a host] [-c channel] [-P panid] [-t node-address-list] [-g 0/1] [-nocli] [device]"
    exit(0)

if __name__ == "__main__":
    # stuff only to run when not called via 'import' here
    manage_device = None
    arg = 1
    start_cli = True

    logging.basicConfig(format='%(asctime)s [%(name)s] %(levelname)s - %(message)s', level=logging.DEBUG)

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
        elif sys.argv[arg] == "-nocli":
            start_cli = False
        else:
            break
        arg += 2

    if len(sys.argv) > arg:
        if sys.argv[arg] == "-h":
            usage()
        if sys.argv[arg].startswith("-"):
            usage()
        manage_device = sys.argv[arg]
        arg += 1

    if len(sys.argv) > arg:
        print "Too many arguments"
        exit(1)

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

    if start_cli:
        dscli.start_cli(server)

    if manage_device:
        server.add_device(manage_device)

    server.serve_forever()
    if server.running:
        server.log.error("*** device server stopped")
