#!/bin/sh
#
# Copyright (c) 2012-2016, Yanzi Networks AB.
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
# Description : Create firmware archive from two images.
#
# ===================================================================
#
#

JAR=jar
DATE=date
CUT=cut
MKTEMP=mktemp
RELBINDIR=`dirname $0`
BINDIR=`(cd $RELBINDIR ; /bin/pwd)`
TRAILER=${BINDIR}/trailertool.py
VARIANT=felicia

while [ -n "$1" ]; do
    case $1 in
        -\? | -h | --help)
            echo "Usage: $PROGNAME [-v variant] [-o output] [flash] [flash2]"
            exit
            ;;
        -o)
            ARCHIVE=$2
            shift
            ;;
        -v)
            VARIANT=$2
            shift
            ;;
        *)
            break
            ;;
    esac
    shift
done

if [ -z "$ARCHIVE" ] ; then
    ARCHIVE=${VARIANT}-firmware.jar
fi

FILEONE=$1
FILETWO=$2

if [ -z "$FILEONE" ] ; then
    FILEONE="${VARIANT}.1.flash"
fi

if [ ! -r "$FILEONE" ] ; then
    echo "Unable to read image 1: \"${FILEONE}\""
    exit 1
fi

if [ -z "$FILETWO" ] ; then
    FILETWO="${VARIANT}.2.flash"
fi

if [ ! -r "$FILETWO" ] ; then
    echo "Unable to read image 2: \"${FILETWO}\""
    exit 1
fi

REV=`git describe --always --abbrev=12 --tags --dirty=M`
if [ -n "$REV" ] ; then
    RELEASENAME=`git tag -l $REV`
fi
HOST=`hostname | $CUT -d. -f1`
BUILDDATE=`$DATE +%s`000
MANIFEST=`$MKTEMP  /tmp/create-image-archive-manifest.XXXXXX`
if [ $? -ne 0 ]; then
    echo "$0: Can't create temp file, exiting..."
    exit 1
fi

STARTADDRESSONE=`$TRAILER -vS -i "$FILEONE" 2>/dev/null`
if [ -z "$STARTADDRESSONE" ] ; then
    echo "Bad start address in file $FILEONE"
    exit 1
fi
STARTADDRESSTWO=`$TRAILER -vS -i "$FILETWO" 2>/dev/null`
if [ -z "$STARTADDRESSTWO" ] ; then
    echo "Bad start address in file $FILETWO"
    exit 1
fi
if [ $STARTADDRESSONE = $STARTADDRESSTWO ] ; then
    echo "Same start address for both image, can not make archive"
    exit 1
fi

IMAGETYPEONE=`$TRAILER -vT -i "$FILEONE" 2>/dev/null`
if [ -z "$IMAGETYPEONE" ] ; then
    echo "Bad image type in file $FILEONE"
    exit 1
fi
IMAGETYPETWO=`$TRAILER -vT -i "$FILETWO" 2>/dev/null`
if [ -z "$IMAGETYPETWO" ] ; then
    echo "Bad image type in file $FILETWO"
    exit 1
fi
if [ $IMAGETYPEONE != $IMAGETYPETWO ] ; then
    echo "image types differ, can not make archive"
    exit 1
fi

PRODUCTTYPEONE=`$TRAILER -vP -i "$FILEONE" 2>/dev/null`
if [ -z "$PRODUCTTYPEONE" ] ; then
    echo "No product type in file $FILEONE"
    exit 1
fi
PRODUCTTYPETWO=`$TRAILER -vP -i "$FILETWO" 2>/dev/null`
if [ -z "$PRODUCTTYPETWO" ] ; then
    echo "No product type in file $FILETWO"
    exit 1
fi
if [ $PRODUCTTYPEONE != $PRODUCTTYPETWO ] ; then
    echo "image types differ, can not make archive"
    exit 1
fi

IMAGEVERSIONONE=`$TRAILER -vV -i "$FILEONE" 2>/dev/null`
if [ -z "$IMAGEVERSIONONE" ] ; then
    echo "Bad image version in file $FILEONE"
    exit 1
fi
IMAGEVERSIONTWO=`$TRAILER -vV -i "$FILETWO" 2>/dev/null`
if [ -z "$IMAGEVERSIONTWO" ] ; then
    echo "Bad image version in file $FILETWO"
    exit 1
fi
if [ $IMAGEVERSIONONE != $IMAGEVERSIONTWO ] ; then
    echo "image version differ, can not make archive"
    exit 1
fi

CHECKSUMONE=`$TRAILER -vC -i "$FILEONE" 2>/dev/null`
if [ -z "$CHECKSUMONE" ] ; then
    echo "Warning: No checksum in $FILEONE"
fi
CHECKSUMTWO=`$TRAILER -vC -i "$FILETWO" 2>/dev/null`
if [ -z "$CHECKSUMTWO" ] ; then
    echo "Warning: No checksum in $FILETWO"
fi

cat > ${MANIFEST} <<EOF
Manifest-Version: 1.0
Attributesversion: 1
Svnversion: $REV
Releasename: $RELEASENAME
Releasedate: $RELEASEDATE
Releasecomment: $RELEASECOMMENT
Builddate: $BUILDDATE
Builder: $USER
Buildhost: $HOST
Imagetype: dualsimpleflash
Producttype: $PRODUCTTYPEONE

Name: $FILEONE
Startaddress: $STARTADDRESSONE
Imagetype: $IMAGETYPEONE
Imageversion: $IMAGEVERSIONONE
Checksum: $CHECKSUMONE

Name: $FILETWO
Startaddress: $STARTADDRESSTWO
Imagetype: $IMAGETYPETWO
Imageversion: $IMAGEVERSIONTWO
Checksum: $CHECKSUMTWO
EOF

$JAR cmf $MANIFEST ${ARCHIVE} "$FILEONE" "$FILETWO"

rm $MANIFEST
