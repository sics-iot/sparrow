This code connects an IEEE 802.15.4 radio over TTY with the full uIPv6
stack of Contiki including 6LoWPAN and 802.15.4 framing / parsing. The
sparrow border router also acts as a RPL Root and handles the routing
and maintains the RPL network. Finally the sparrow border router
connects the full 6LoWPAN/RPL network to the host (Linux/OS-X) network
stack making it possible for applications on the host to transparently
reach all the nodes in the 6LoWPAN/RPL network.

This is designed to interact with the sparrow serial-radio running on
a mote that is either connected directly via USB/TTY, or remote via a
TCP connection.

The border router supports a number of commands on it's stdin.

* routes - list routes
* nbr    - list neighbors
* !G     - global RPL repair root.

* ?C is used for requesting the currently used channel for the serial-radio. The response is !C with a channel number (from the serial-radio).

* !C is used for setting the channel of the serial-radio (useful if the motes are using another channel than the one used in the serial-radio).

Starting the border router

  > cd products/sparrow-border-router
  > make
  > make connect-router

If the serial radio is connected to a specific port not selected as
first port by the border router:

  > make connect-router PORT=/dev/ttyACM0
