#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

. $DIR/params.sh

PACKAGE_NAME="newlib-1.19.0"
FILE_NAME=$PACKAGE_NAME".tar.gz"
[ -f $TAR_DIR/$FILE_NAME ] || wget "ftp://sourceware.org/pub/newlib/$FILE_NAME" -O $TAR_DIR/$FILE_NAME
if [ ! -d $SRC_DIR/$PACKAGE_NAME ]; then
    pushd $SRC_DIR
    tar -xf $TAR_DIR/$FILE_NAME -C $SRC_DIR
    patch -p1 < $DIR/patches/$PACKAGE_NAME.patch
    popd
fi

cp -r $DIR/patches/$PACKAGE_NAME/* $SRC_DIR/$PACKAGE_NAME
pushd $SRC_DIR/$PACKAGE_NAME/newlib/libc/sys/moo
autoreconf2.64
cd ..
autoconf2.64
popd

[ -d $BUILD_DIR/$PACKAGE_NAME ] || mkdir -p $BUILD_DIR/$PACKAGE_NAME
pushd $BUILD_DIR/$PACKAGE_NAME
$SRC_DIR/$PACKAGE_NAME/configure --target=$TARGET --prefix=/var/moo --exec-prefix=/var/moo CFLAGS='-O0 -ggdb3' CXXFLAGS='-O0 -ggdb3'
make
make install
popd
