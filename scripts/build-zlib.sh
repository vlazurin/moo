#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

. $DIR/params.sh

PACKAGE_NAME="zlib-1.2.11"
FILE_NAME=$PACKAGE_NAME".tar.gz"
[ -f $TAR_DIR/$FILE_NAME ] || wget "http://zlib.net/$FILE_NAME" -O $TAR_DIR/$FILE_NAME
if [ ! -d $SRC_DIR/$PACKAGE_NAME ]; then
    pushd $SRC_DIR
    tar -xf $TAR_DIR/$FILE_NAME -C $SRC_DIR
    popd
fi

[ -d $BUILD_DIR/$PACKAGE_NAME ] || mkdir -p $BUILD_DIR/$PACKAGE_NAME
pushd $SRC_DIR/$PACKAGE_NAME
CC=i386-pc-moo-gcc ./configure --static --prefix=$PREFIX
make
make DESTDIR=$SYSROOT install
popd
