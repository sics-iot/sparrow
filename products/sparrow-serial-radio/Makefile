SPARROW=../..
CONTIKI_PROJECT=serial-radio

all: $(CONTIKI_PROJECT)

APPS = slip-cmd sparrow-oam sparrow-beacon
APPS += sparrow-instances/instance-flash
CFLAGS += -Werror -Wno-error=tautological-compare

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
  ifdef MAKE_WITH_USB_PORT
    CFLAGS += -DWITH_USB_PORT=1
  endif
endif

ifeq ($(BOARD),cc2538em)
  ifdef MAKE_WITH_UART
    CFLAGS += -DWITH_UART=1
  else ifdef MAKE_WITH_UART_FLOW_CONTROL
    CFLAGS += -DWITH_UART=1
    CFLAGS += -DWITH_UART_FLOW_CONTROL=1
  endif
endif

#linker optimizations
SMALL=1

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += enc-net.c no-framer.c sniffer-rdc.c radio-scan.c

CONTIKI_WITH_RPL = 0
CONTIKI_WITH_IPV6 = 1
include $(SPARROW)/Makefile.sparrow
