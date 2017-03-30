SPARROW=../..
CONTIKI_PROJECT = dual-op-mode

all: $(CONTIKI_PROJECT)

APPS += slip-cmd
APPS += sparrow-oam sparrow-beacon
APPS += sparrow-instances/instance-flash
ifneq ($(BOARD),cc2538em)
APPS += sparrow-instances/instance-leds
endif
APPS += sparrow-instances/instance-nstats

ifdef MAKE_WITH_NETSCAN
APPS += netscan
endif

ifeq ($(TARGET),)
  -include Makefile.target
endif

ifeq ($(TARGET),)
 ${error The serial radio can not be built for the native platform.}
endif

ifeq ($(TARGET),native)
 ${error The serial radio can not be built for the native platform.}
endif

ifeq ($(TARGET),native-sparrow)
 ${error The serial radio can not be built for the native platform.}
endif

# Include sourcefiles from serial radio
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
SPARROW_OAM_INSTANCES += instance_radio
PROJECT_SOURCEFILES += instance-radio.c transmit-buffer.c
PROJECT_SOURCEFILES += enc-net.c no-framer.c sniffer-rdc.c radio-scan.c
PROJECT_SOURCEFILES += serial-radio.c
# Include proxy modules
PROJECT_SOURCEFILES += dual-mode-net.c dual-mode-rdc.c

CONTIKIDIRS += ../sparrow-serial-radio

CONTIKI_WITH_IPV6 = 1
include $(SPARROW)/Makefile.sparrow
