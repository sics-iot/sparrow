Getting Started with Contiki for Yanzi Felicia
==============================================

This guide's aim is to help you start using Contiki for Yanzi Felicia nodes.
The platform supports various types of boards based on TI CC2538 System-on-Chip.

* Felicia - the base board
* IoT-U10 - board with leds, buttons, temperature sensor
* IoT-U10+ - board with leds, buttons, temperature sensor, and CC2592 Range Extender.
* DonkeyJr - extension board with Ethernet, temperature sensor, etc

Requirements
============

To start using Contiki, you will need the following:

* A toolchain to compile for CC2538.

* Drivers so that your OS can communicate with the hardware via USB. Many OS already have the drivers pre-installed while others might require you to install additional drivers. Please read the section ["Drivers" in the CC2538DK readme](https://github.com/contiki-os/contiki/tree/master/platform/cc2538dk#for-the-cc2538em-usb-cdc-acm). Only the driver for CC2538EM (USB CDC-ACM) is needed.


Use the platform
================

Starting border router
--------------------------------------------------

To update a node using the over-the-air upgrade mechanism, it needs to be connected in an IPv6 network.
The border router with support for OTA is under products/sparrow-border-router and it needs to be started to setup a network for the nodes to join.

    > cd products/sparrow-border-router
    > make connect-high

You need to have an updated IoT-U10/IoT-U10+ node with the serial radio
software connected via USB. You might need to specify a different USB
port if the node is not connected to the first port.

    > make connect-high PORT=/dev/ttyACM1

Build the image of the remote mote
--------------------------------------------------

Switch to the folder of the project you want to build. For example, to
build an image for the IoT-U10 node, switch to examples/felicia/iot-u10/
The Makefile contains all the necessary information for building the
image(s). You need the following options when building the images:

 INC: The sequence number of the images. When doing an over-the-air
 upgrade, the device will reboot to the latest image based on date. If
 both images have the same date, the device will boot to the image
 with the highest INC number. The default INC number when not
 specified is 0.

Example:

    > cd examples/felicia/iot-u10/
    > make clean images INC=3

The image archive is created in the working directory and named
"felicia-firmware.jar".

Over the air upgrade
--------------------------------------------------

There is a convenient make rule to upload to a node:

    > make images
    > make upload DST=<fd02::XXXX:XXXX:XXXX>

DST should be the IPv6 address to the node to update. After the update, the node will reboot and at startup it will blink once with the yellow led if it boots on the first image and twice if it boots on the second image. To see the status on a node and which image is currently used:

    > make status DST=<fd02::XXXX:XXXX:XXXX>

A locally connected node can be programmed faster:

    > make upload-fast DST=<fd02::XXXX:XXXX:XXXX>

Over the air upgrade tool
--------------------------------------------------

The tool for over the air upgrade is located under /tools/sparrow and is
a JAR file. The command to start the over-the-air upgrade is as
follows:

    > ../../../tools/sparrow/tlvupgrade.py -i [the image archive] -a fd02::XXXX:XXXX:XXXX

You will be able to monitor the update process in your terminal. The
upgrade performs a write of the image and then checks the image
status. If the status check succeeds, the device will reboot and the
bootloader will boot from the newest image. If the status check fails,
the device will not automatically reboot and will continue to run
using the old image.

If you only want to check the image status of the node, you type:

    > ../../../tools/sparrow/tlvupgrade.py -s -a fd02::XXXX:XXXX:XXXX

There is a nice "usage" command on this Python script, so you can see how,
for example you can change the speed of the upgrade process (using the
-b option and modifying the block size etc.)

Note that over-the-air upgrade assumes that you have a border router
running (so the tun interface is up) and the remote mote has joined
the RPL instance of the border router. For that, the serial radio must
be tuned to the channel and PAN id of the remote mote. By default
channel 26 and PAN id 0xabcd are used by both the serial radio and the
remote Yanzi mote products (IoT-U10, IoT-U10+, etc.).

To verify connectivity, try to ping the remote mote before attempting
a firmware upgrade.

Updating the serial radio
------------------------------------------------------------

The serial radio and border router with support for OTA is under
products/sparrow-serial-radio and products/sparrow-border-router.
To use these you need an updated IoT-U10/IoT-U10+ serial radio.

For felicia platform, you must specify the target and the board,
e.g. iot-u10 or iot-u10plus.

    > cd products/sparrow-serial-radio
    > make TARGET=felicia BOARD=iot-u10 clean images INC=1
    > make TARGET=felicia BOARD=iot-u10 upload-fast DST=127.0.0.1

There is a convenient make rule for updating a locally connected
serial radio with high speed:

    > make TARGET=felicia BOARD=iot-u10 upload-local

To upgrade you need to have the border router running and connected to
the serial-radio.
