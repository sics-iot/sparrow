Zolertia Zoul core module with Sparrow support
==============================================

This is a platform setup to use Zolertia Zoul platform together with
Sparrow Application Layer. It is based on the Zolertia Zoul platform
in the Contiki OS. Please see the README in Contiki OS platform Zoul
for more information about and requirements to use Zolertia Zoul:
`contiki/platform/zoul/README.md`.

Use the platform
================

Starting border router
--------------------------------------------------

To update a node using the over-the-air upgrade mechanism, it needs to be connected in an IPv6 network.
The border router with support for OTA is under products/sparrow-border-router and it needs to be started to setup a network for the nodes to join.

    > cd products/sparrow-border-router
    > make connect-router

You need to have an updated Zolertia Zoul RE-MOTE node with the serial radio
software connected via USB. You might need to specify a different USB
port if the node is not connected to the first port.

    > make connect-router PORT=/dev/ttyACM1

Build the image of the remote mote
--------------------------------------------------

Switch to the folder of the project you want to build. For example, to
build an image for the RE-MOTE node, switch to examples/zoul/remote/
The Makefile contains all the necessary information for building the
image(s). You need the following options when building the images:

 INC: The sequence number of the images. When doing an over-the-air
 upgrade, the device will reboot to the latest image based on date. If
 both images have the same date, the device will boot to the image
 with the highest INC number. The default INC number when not
 specified is 0.

Example:

    > cd examples/zoul/remote/
    > make clean images INC=3

The image archive is created in the working directory and named
"zoul-sparrow-firmware.jar".

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
upgrade performs a write of the images and then checks the image
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
remote mote products (Zoul RE-MOTE, etc.).

To verify connectivity, try to ping the remote mote before attempting
a firmware upgrade.

Updating the serial radio
------------------------------------------------------------

The serial radio and border router with support for OTA is under
products/sparrow-serial-radio and products/sparrow-border-router.
To use these you need an updated Zoul RE-MOTE serial radio.

For Zoul platform, you must specify the target and the board,
e.g. RE-MOTE or Firefly.

    > cd products/sparrow-serial-radio
    > make TARGET=zoul-sparrow BOARD=remote clean images INC=1
    > make TARGET=zoul-sparrow BOARD=remote upload-fast DST=127.0.0.1

There is a convenient make rule for updating a locally connected
serial radio with high speed:

    > make TARGET=zoul-sparrow BOARD=remote upload-local

To upgrade you need to have the border router running and connected to
the serial-radio.
