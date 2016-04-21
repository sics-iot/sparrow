#
#
# makefile for M3 bootloader.
#
#
# Define tools used
TOOL=arm-none-eabi-
CC=$(TOOL)gcc
AS=$(TOOL)gcc
AR=$(TOOL)ar
LD=$(TOOL)ld
OBJCOPY=$(TOOL)objcopy
SIZE=$(TOOL)size
SRCS=
ASRCS=
CFLAGS=
OBJECTDIR=.

#IMAGE=0

all: cc2538-image.ld bootloader

#
# Contiki source where we can find trailer.h
SPARROW=../../..
CONTIKI=$(SPARROW)/contiki
CONTIKI_CPU=$(SPARROW)/cpu/cc2538
IMAGE_TRAILER=$(SPARROW)/lib/image-trailer
SOURCEDIRS=. $(CONTIKI_CPU) $(CONTIKI_CPU)/dev $(SPARROW)/core $(CONTIKI)/core
include $(CONTIKI_CPU)/Makefile.cc2538


# The target application name.
BOOTLOADER=bootloader.elf

CPPFLAGS=-MD
CFLAGS+=-mthumb -mcpu=cortex-m3
CFLAGS+=-I.
CFLAGS+=-I$(SPARROW)/core
CFLAGS+=-I$(CONTIKI)/core
CFLAGS+=-I$(CONTIKI_CPU)
CFLAGS+=-I$(CONTIKI_CPU)/dev
CFLAGS+=-I$(IMAGE_TRAILER)
CFLAGS+=-DIMAGE_TRAILER_MEMCMP=rom_util_memcmp
#CFLAGS+=-g -Wall -Werror -Wundef -Os -O2
CFLAGS+=-g -Wall -Werror -Os -O2

SRCS+=bootloader.c
SRCS+=startup-gcc.c $(CONTIKI_CPU)/crc32.c
SRCS+=$(IMAGE_TRAILER)/image-trailer.c

# Convert source name to object and dependencies
OBJS=$(SRCS:.c=.o) $(ASRCS:.s79=.o)
DEPS=$(SRCS:.c=.d) $(ASRCS:.s79=.d)

# Linker options
LINKEROPTS=-mthumb
LINKEROPTS+=-mcpu=cortex-m3
LINKEROPTS+=-T cc2538-image.ld
LINKEROPTS+=-static
LINKEROPTS+=-Wl,-cref,-u,Reset_Handler
LINKEROPTS+=-Wl,--gc-sections

# The actual rules to build it all

%.o: %.s79
	$(AS) $(ASFLAGS) -o $@ $<

bootloader: $(BOOTLOADER)
	$(OBJCOPY) -O binary $(BOOTLOADER) bootloader.odd
	dd if=/dev/zero of=pad bs=1 count=`ls -l bootloader.odd | awk ' { pad = 0; size = $$5 ; rest = size % 16 ; if (rest > 0) { pad = 16 - rest;}  print pad }'` > /dev/null
	cat bootloader.odd pad > bootloader
	rm bootloader.odd pad


$(BOOTLOADER): $(OBJS)
	$(CC) -o $@ $(LINKEROPTS) -Wl,-Map=$(@:.elf=.map) $(OBJS)
	$(SIZE) -A -x $@
	$(SIZE) -B -x $@


clean:
	rm -f $(BOOTLOADER) $(BOOTLOADER:.elf=.map) $(OBJS) $(OBJS:.o=.d)
	rm -f cc2538-image.ld bootloader cca.bin

# include all dependencies (softly; no error if missing)
-include $(DEPS)
