SPARROW=../..
CONTIKI=$(SPARROW)/contiki
CONTIKI_PROJECT=border-router

ifndef HOST_OS
  ifeq ($(OS),Windows_NT)
    HOST_OS := Windows
  else
    HOST_OS := $(shell uname)
  endif
endif

all: $(CONTIKI_PROJECT)

APPS += slip-cmd
APPS += sparrow-oam sparrow-instances/instance-nstats

SPARROW_OAM_INSTANCES += radio1 radio2 radio3 instance_rtable instance_brm
CURSES_LIBS=

TARGET=native-sparrow

#linker optimizations
SMALL=1

CFLAGS += -Werror
# OSX (g)cc does not like  -Wunused-but-set-variable
ifneq ($(HOST_OS),Darwin)
 CFLAGS += -Wunused-but-set-variable
endif
CFLAGS += -Wunused
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CONTIKI_SOURCEFILES += border-router-cmds.c tun-bridge.c border-router-rdc.c \
border-router-radio.c br-config.c enc-dev.c border-router-ctrl.c \
border-router-server.c dataqueue.c latency-stats.c ylog.c

CFLAGS += -DHAVE_BORDER_ROUTER_CTRL=1
CFLAGS += -DHAVE_BORDER_ROUTER_SERVER=1

CONTIKI_SOURCEFILES += udp-cmd.c instance-rtable.c instance-brm.c

ifneq ($(WITH_CLOCK_GETTIME),)
  CFLAGS += -DWITH_CLOCK_GETTIME=$(WITH_CLOCK_GETTIME)
endif

WITH_WEBSERVER=1
ifeq ($(WITH_WEBSERVER),1)
CFLAGS += -DWEBSERVER=1
PROJECT_SOURCEFILES += httpd-simple.c
else ifneq ($(WITH_WEBSERVER), 0)
APPS += $(WITH_WEBSERVER)
CFLAGS += -DWEBSERVER=2
endif

CC=$(CROSS_COMPILE)gcc
LD_OVERRIDE=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar

CONTIKI_WITH_IPV6 = 1
include $(SPARROW)/Makefile.sparrow
ifneq ($(HOST_OS),Darwin)
  LDFLAGS += -pthread
endif
ifeq ($(HOST_OS),Linux)
  LDFLAGS += -lrt
endif

ifdef PORT
  PORTARGS = -s $(PORT)
else ifeq ($(HOST_OS),Darwin)
  PORT = ${firstword ${wildcard /dev/tty.usbserial-* /dev/tty.usbmodem* /dev/tty.SLAB_USBtoUART}}
  ifneq ($(PORT),)
    PORTARGS = -s $(PORT)
  endif
endif

ifndef PREFIX
  PREFIX=fd02::1/64
endif

ifdef WITH_GDB
  ifeq ($(HOST_OS),Darwin)
    RUN_GDB=lldb --
  else
    RUN_GDB=gdb -ex run --args
  endif
endif

connect-router:	connect-normal

connect-normal: border-router.$(TARGET)
	sudo $(RUN_GDB) ./border-router.$(TARGET) $(PORTARGS) ${PREFIX}

connect-full:	border-router.$(TARGET)
	sudo $(RUN_GDB) ./border-router.$(TARGET) $(PORTARGS) -B 460800 ${PREFIX}

connect-high:	border-router.$(TARGET)
	sudo $(RUN_GDB) ./border-router.$(TARGET) $(PORTARGS) -B 921600 ${PREFIX}

connect-slave:	border-router.$(TARGET)
	$(RUN_GDB) ./border-router.$(TARGET) $(PORTARGS) -S

connect-slave-high:	border-router.$(TARGET)
	$(RUN_GDB) ./border-router.$(TARGET) $(PORTARGS) -B 921600 -S
