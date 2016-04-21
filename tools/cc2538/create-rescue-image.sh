#!/bin/sh
#
# Copyright (c) 2014-2016, Yanzi Networks AB.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#    3. Neither the name of the copyright holders nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# ===================================================================
#
# Description : Create a rescue image with bootloader.
#
# ===================================================================
#

usage() {
    echo "Usage: $0 -B <bootloader binary> [-m <mfg hex> | -M >mfg bin>] -i <firmware binary [-o output binary]"
    echo "        -m use hex file to create mfg area"
    echo "        -M use pre-made mfg area binary file"
    echo "        if no mfg area specified, it is assumed to be included in bootloader"
    echo "        -i firmware image to include"
    echo "        -B bootloader binary to include"
    echo "        -D set serial bootloader backdoor disabled in CCA area"
}


WHERE=`dirname $0`
CONTIKI_DIR=`(cd ${WHERE}/../.. 2> /dev/null; /bin/pwd)`
BOOTLOADER_DIR=""
VARIANT=""
FIRMWARE=""
BOOTLOADER_BINARY=""
REBUILD_BOOTLOADER=no
DISABLE_SERIAL_BOOTLOADER=no
OUTPUT=rescueimage.bin
MFGHEX=""
MFGBIN=""

BINFILETOOL="${WHERE}/../sparrow/binfiletool.py"

FIRSTIMAGE_START=16384
CCA_START=524240

while [ -n "$1" ] ; do
    case $1 in
        -\? | -h | --help)
            usage
            exit
            ;;
        -b | --bootloader)
            BOOTLOADER_DIR=$2
            shift
            ;;
        -c | --contiki)
            CONTIKI_DIR=$2
            shift
            ;;
        -m | --mfghex)
            MFGHEX=$2
            shift
            ;;
        -M | --mfgbin)
            MFGBIN=$2
            shift
            ;;

        -i | --firmware)
            FIRMWARE=$2
            shift
            ;;
        -o | --output)
            OUTPUT=$2
            shift
            ;;
        -B)
            BOOTLOADER_BINARY=$2
            shift
            ;;
        -D)
            DISABLE_SERIAL_BOOTLOADER=yes
            ;;
        *)
            usage
            echo "Unknown option \"$1\""
            exit
            ;;
    esac
    shift
done

if [ ! -r "$BOOTLOADER_BINARY" ] ; then
    echo "invalid bootloader binary \"${BOOTLOADER_BINARY}\""
    usage
    exit
fi
BOOTLOADERSIZE=`wc -c ${BOOTLOADER_BINARY} | awk '{print $1}'`
if [ $BOOTLOADERSIZE -gt $FIRSTIMAGE_START ] ; then
    echo "bootloader is too big, ${BOOTLOADERSIZE} bytes. Compiled without IMAGE=0 ?"
    exit
fi
if [ ! -r "$FIRMWARE" ] ; then
    echo "invalid firmware binary \"${FIRMWARE}\""
    usage
    exit
fi

if [ -r "${MFGHEX}" -a -r "${MFGBIN}" ] ; then
    echo "Specify only one of mfg hex and mfg bin"
    usage
    exit
fi

if [ -z "${OUTPUT}" ] ; then
    echo "Specify only an output file for resulting binary"
    usage
    exit
fi

MFGTMPBIN=`mktemp mfg-XXXXXXXXX`
if [ -r ${MFGHEX} ] ; then
    echo "Creating binary mfg from hex"
    ${BINFILETOOL} -i $MFGHEX -o ${MFGTMPBIN}
elif [ -r ${MFGBIN} ] ; then
    cat ${MFGBIN} > ${MFGTMPBIN}
else
    rm {$MFGTMPBIN}
fi

if [ "${DISABLE_SERIAL_BOOTLOADER}" = "yes" ] ; then
  echo "Creating cca area (serial bootloader disabled)"
  BOOTLOADERBACKDOOR="e3"
else
  echo "Creating cca area (serial bootloader enabled)"
  BOOTLOADERBACKDOOR="f3"
fi

CCA=`mktemp cca-XXXXXXXXX`
cat << EOF | $BINFILETOOL -o ${CCA}
ff ff ff ff ff ff ff ${BOOTLOADERBACKDOOR} 00 00 00 00 00 00 20 00
ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff
EOF

TMP1FILE=`mktemp rescue-XXXXXXXXX`
TMP2FILE=`mktemp rescue-XXXXXXXXX`

echo "Adding bootloader and padding"
if [ -r "${MFGTMPBIN}" ] ; then
    cat ${BOOTLOADER_BINARY} ${MFGTMPBIN} > ${TMP2FILE}
    $BINFILETOOL -B -i ${TMP2FILE} -o ${TMP1FILE} -p -${FIRSTIMAGE_START}
else
    $BINFILETOOL -B -i ${BOOTLOADER_BINARY} -o ${TMP1FILE} -p -${FIRSTIMAGE_START}
fi

echo "Adding firmware image"
cat ${TMP1FILE} ${FIRMWARE} > ${TMP2FILE}
$BINFILETOOL -B -i ${TMP2FILE} -o ${TMP1FILE} -p -${CCA_START}
echo "Adding cca"
cat ${TMP1FILE} ${CCA} > ${OUTPUT}

rm ${MFGTMPBIN} ${TMP1FILE} ${TMP2FILE} ${CCA}

echo "Created file ${OUTPUT}"
