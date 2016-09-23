SPARROW=../../..
CONTIKI=$(SPARROW)/contiki
CONTIKI_PROJECT = iot-u10

TARGET=felicia
BOARD=iot-u10

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

all: $(CONTIKI_PROJECT)

APPS += sparrow-oam
APPS += sparrow-instances/instance-flash
APPS += sparrow-instances/stts751
APPS += sparrow-instances/instance-leds
APPS += sparrow-instances/instance-button
APPS += sparrow-instances/instance-nstats
APPS += er-coap
APPS += rest-engine
APPS += mote-res/resources-coap
APPS += mote-res/resources-common

ifdef WITH_NETSCAN
APPS += netscan
endif

ifdef WITH_WEBSERVER
APPS += mote-res/resources-web
endif

CONTIKI_WITH_IPV6 = 1
include $(SPARROW)/Makefile.sparrow
