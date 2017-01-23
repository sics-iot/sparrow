#!/usr/bin/env python
#
# Copyright (c) 2016, Yanzi Networks AB.
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
#

import readline,threading
COMMANDS = ['list', 'wakeup', 'exit', 'bye', 'quit', 'q']
deviceServer = None

def complete(text, state):
    index = readline.get_begidx()
    if index == 0:
        for cmd in COMMANDS:
            if cmd.startswith(text):
                if not state:
                    return cmd
                else:
                    state -= 1
    else:
        if deviceServer:
            devices = deviceServer.get_devices()
            for dev in devices:
                if dev.address.startswith(text):
                    if not state:
                        return dev.address
                    state -= 1

def cli_loop():
    global deviceServer
    running = True
    while running:
        try:
            inp = raw_input("> ")
            cmds = inp.split(" ")
            cmd = cmds[0]
            if cmd == "list":
                devices = deviceServer.get_devices()
                if devices:
                    for dev in devices:
                        print dev.info()
                else:
                    print "No devices to list"
            elif cmd == "exit" or cmd == "quit" or cmd == "q" or cmd == "bye":
                deviceServer.stop()
                running = False
            elif cmd == "wakeup":
                if len(cmds) == 1:
                    print "Usage: wakeup <device> [time-in-seconds]"
                elif len(cmds) > 2:
                    deviceServer.wakeup(cmds[1], int(cmds[2]))
                else:
                    deviceServer.wakeup(cmds[1])
        except Exception as e:
            print "Error:", e


def start_cli(server = None):
    global deviceServer
    if 'libedit' in readline.__doc__:
        readline.parse_and_bind("bind ^I rl_complete")
    else:
        readline.parse_and_bind("tab: complete")

    readline.set_completer(complete)

    deviceServer = server

    t = threading.Thread(target=cli_loop)
    t.daemon = True
    t.start()
    return t


# test if run as main.
if __name__ == "__main__":
    t = start_cli()
    t.join()
