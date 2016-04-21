# Sparrow Application Layer and Tools

This repository contain a full implementation of the Sparrow
application layer protocol and tools needed to make use of it. The
implementation is based on Contiki OS and some of the popular
platforms are supported from start.

Sparrow is communicating on top of IPv6/UDP and defines an encapsulation
format (called encap) that encapsulates different types of
payload. The most important payload is the TLV payload. This is used
for all of the application data to and from IoT devices running
Sparrow. TLVs consist of type, length and values and are fairly
similar to OMA TLVs used in OMA LWM2M.

The application layer also defines an object model where each IoT
device can present a number of object instances at runtime. There is a
discovery procedure to discover the number of objects that any IoT
device implements and their type. All Sparrow devices have instance
0 which is a management and discovery instance. Instance 0 includes one
variable that describes the number of instances available on this
device and knowing the instance count, all other instances can be
discovered (e.g. read out type of object and other parameters).

# License

Sparrow Application Layer is released under BSD 3-Clause License.

# Contributions

Design by Yanzi Networks.
Main contributors:

Lars Ramfelt <lars.ramfelt@yanzinetworks.com>  
Stefan Sandhagen <stefan.sandhagen@yanzinetworks.com>  
Fredrik Ljungberg <flag@yanzinetworks.com>  
Oriol Piñol Piñol <oriol.pinol@yanzinetworks.com>  
Marie Lassborn <marie.lassborn@yanzinetworks.com>  
Simon Gidlund <simon.gidlund@yanzinetworks.com>  

The work on preparing the Sparrow Application Layer
for open source release have been done by Niclas Finne
and Joakim Eriksson, SICS Swedish ICT (nfi@sics.se and
joakime@sics.se).

# Using Sparrow

Start by checking out needed submodules. Contiki OS is included as a
git submodule and some of the Contiki OS submodules are also needed.

    > cd sparrow
    > git submodule update --init --recursive

# Using Sparrow OTA with Zolertia Zoul RE-MOTE

There is an example application included for testing with Zolertia
RE-MOTE. To begin with, the RE-MOTE needs to be programmed with a
rescue image that contains the Sparrow bootloader. The RE-MOTE can
always be reprogrammed again using the serial bootloader so it is no
problems reprogramming the RE-MOTE later with another non-Sparrow
application.

## Program a Zolertia Zoul RE-MOTE with rescue-image

    > cd examples/zoul/remote
    > make IMAGE=1
    > make rescue-image

Make sure the RE-MOTE is connected via USB

    > make upload-rescue-image

Now the RE-MOTE should be running an example application where it is
possible to control the LEDs and listen to the user button using the
Sparrow application layer. Make sure you have a border router running
and wait for the RE-MOTE to connect to your network before trying to
communicate with the node.

## Program a Zolertia Zoul RE-MOTE over-the-air.

Start by creating an image archive for OTA. An image archive contains
both images which makes it easy for the upload tool to simply upload
the image for the container that is currently not used in the node.

**NOTE: Remember to always increase the INC number when recompiling
during each day to mark the newly compiled image as newer than the
previous image. If both images have the same date and same INC, the
first one will be used when booting.**

     > cd examples/zoul/remote
     > make images INC=0
     > make upload-fast DST=<IP-address-of-the-RE-MOTE-node>

The make rule `upload-fast` can be used for faster update of locally
connected (single hop) nodes. The make rule `upload` uses a slower
update process that should be used for multihop networks or to avoid
affecting the network during updates.

During bootup, before the application is started, the Zoul RE-MOTE
will blink once if booting on the first image, and twice if booting on
the second image.
