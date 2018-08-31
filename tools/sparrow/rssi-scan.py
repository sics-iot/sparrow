#!/usr/bin/env python2.7
#
# Copyright (c) 2013-2018, Yanzi Networks
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
# RSSI scanner for Sparrow Serial Radio and Sparrow Border Router
# that scans a single radio channel and log the RSSI values.
#

import sys, getopt, os.path
import serialradio
import struct, time

import matplotlib.pyplot as plt

def usage():
    print sys.argv[0],"[-c <channel>] [-t <time-in-seconds>] [-i <inputfile>] [-o <outputfile>] [-a host] [-p port] [-P]"

def plot_data(fig, ax, data):
    ax.clear()
    ax.plot(range(0,len(data)), data, label='RSSI')
    ax.set(xlabel='Sample', ylabel='RSSI (dBm)', title='rssi')
    ax.legend(bbox_to_anchor=(1, 1))
    ax.set_ylim([-127, 0])
    ax.grid()

#
# Read command line arguments
#
infile = None
outfile = None
channel = None
end_time = None
host = "localhost"
port = 9999
plot = False
plotmax = None
rssiarr = []

try:
    argv = sys.argv[1:]
    opts, args = getopt.getopt(argv,"hfi:o:c:t:m:a:p:P")
except getopt.GetoptError as e:
    sys.stderr.write("Error: {}\n".format(e))
    usage()
    sys.exit(2)
for opt, arg in opts:
    if opt == '-h':
        usage()
        print "Connect to a native border router and use it as a radio RSSI scanner."
        sys.exit()
    elif opt == "-i":
        infile = arg
    elif opt == "-o":
        outfile = arg
    elif opt == "-c":
        channel = arg
    elif opt == "-t":
        end_time = time.time() + int(arg)
    elif opt == "-m":
        plotmax = int(arg)
    elif opt == "-a":
        host = arg
    elif opt == "-p":
        port = int(arg)
    elif opt == "-P":
        plot = True

if infile is not None:
    # Read from file
    if not os.path.isfile(infile):
        sys.stderr.write("Could not read file {}\n".format(infile))
        sys.exit(2)
    with open(infile) as f:
        for line in f:
            line = line.strip()
            if line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) > 4:
                for p in parts[4:]:
                    rssiarr.append(int(p))

    if plotmax is not None and len(rssiarr) > plotmax:
        del rssiarr[:len(rssiarr) - plotmax]

    fig, ax = plt.subplots()
    plot_data(fig, ax, rssiarr)
    fig.canvas.set_window_title(infile)
    plt.show()
    sys.exit(0)

con = serialradio.SerialRadioConnection()
con.connect(host=host, port=port)

if outfile is not None:
    out = open(outfile, "w")
else:
    out = sys.stdout

if plot:
    # Note that using plt.subplots below is equivalent to using
    # fig = plt.figure and then ax = fig.add_subplot(111)
    fig, ax = plt.subplots()
    plot_data(fig, ax, rssiarr)
    plt.ion()
    # Use a default max number values when using real time plots
    if plotmax is None:
        plotmax = 20000

if outfile is not None:
    sys.stderr.write("Configuring radio for scanning (border router will not work during scanning)\n")
    sys.stderr.write("Writing log to " + str(outfile) + "\n")
if channel is not None:
    con.send("!C" + struct.pack("B", int(channel)))
con.send("!m\003")
# Set the single-channel scan
con.send("!c\002")
i = 0
sample = 0

out.write("#start: {}\n".format(time.strftime("%Y-%m-%d %H:%M:%S")))
if channel is not None:
    out.write("#channel: {}\n".format(channel))
out.write("#format: ms_since_start radio_sample diff local_sample [RSSI...]\n")
start_timestamp = int(round(time.time() * 1000))
t = 0
while True:
    if end_time is not None and end_time < time.time():
        break

    try:
        frame = con.get_next_frame(10)
    except KeyboardInterrupt:
        break

    if frame is not None:
        ts = frame.timestamp - start_timestamp
        data = frame.get_encap()
        if data.serial_data[0] == '!' and data.serial_data[1] == 'r' and data.serial_data[2] == 'C':
            # print str(data.serial_data[0:3]), struct.unpack("!H", data.serial_data[3:5]),
            i += 1
            ot = t
            t = struct.unpack("!H",data.serial_data[3:5])[0]
            out.write("{:>5} {:>5} {:>2} {:>5}".format(int(round(time.time() * 1000) - start_timestamp), t, (t - ot) & 0xffff, sample))
            for x in data.serial_data[5:]:
                # fake conversion - assumes RSSI is always negative...
                rssi_value = ord(x) - 256
                out.write(" {:>4}".format(rssi_value))
                if plot:
                    rssiarr.append(rssi_value)
                sample += 1
            out.write("\n")

            if outfile is not None:
                if i % 78 == 0:
                    sys.stderr.write('.\n')
                else:
                    sys.stderr.write('.')
            if plot and i % 20 == 0:
                if plotmax is not None and len(rssiarr) > plotmax:
                    del rssiarr[:len(rssiarr) - plotmax]

                plot_data(fig, ax, rssiarr)
                fig.canvas.draw()

    if plot:
        try:
            plt.pause(0.001)
        except KeyboardInterrupt:
            break

out.write("#end: {}\n".format(time.strftime("%Y-%m-%d %H:%M:%S")))
if outfile is not None:
    out.close()
con.close()
sys.stderr.write("[connection closed]\n")
