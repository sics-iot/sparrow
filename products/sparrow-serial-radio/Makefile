SPARROW=../..
CONTIKI=$(SPARROW)/contiki
CONTIKI_PROJECT=serial-radio

all: $(CONTIKI_PROJECT)

APPS = slip-cmd sparrow-oam sparrow-beacon
APPS += sparrow-instances/instance-flash
CFLAGS += -Werror

SPARROW_OAM_LOCAL_INSTANCES += instance_radio

PROJECT_SOURCEFILES += instance-radio.c transmit-buffer.c

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

ifeq ($(TARGET),zoul-sparrow)
  ifdef USE_DEBUG_PORT
    CFLAGS += -DZOUL_SPARROW_USB_DEBUG_PORT
  endif
endif

#linker optimizations
SMALL=1

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += enc-net.c no-framer.c sniffer-rdc.c radio-scan.c

CONTIKI_WITH_RPL = 0
CONTIKI_WITH_IPV6 = 1
include $(SPARROW)/Makefile.sparrow
