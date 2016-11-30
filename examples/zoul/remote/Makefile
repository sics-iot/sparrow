SPARROW=../../..
CONTIKI=$(SPARROW)/contiki
CONTIKI_PROJECT = remote

TARGET=zoul-sparrow
BOARD=remote

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

all: $(CONTIKI_PROJECT)

APPS += sparrow-oam
APPS += sparrow-instances/instance-flash
APPS += sparrow-instances/instance-temperature
APPS += sparrow-instances/instance-leds
APPS += sparrow-instances/instance-button
APPS += sparrow-instances/instance-nstats
APPS += sparrow-er-coap
APPS += rest-engine
APPS += mote-res/resources-coap
APPS += mote-res/resources-common

ifdef MAKE_WITH_WEBSERVER
APPS += mote-res/resources-web
endif

ifdef MAKE_WITH_IPSO
APPS += oma-lwm2m
APPS += ipso-objects
CONTIKI_SOURCEFILES += platform-ipso-temperature.c
CFLAGS += -DWITH_IPSO=1
endif

CONTIKI_WITH_IPV6 = 1
include $(SPARROW)/Makefile.sparrow
