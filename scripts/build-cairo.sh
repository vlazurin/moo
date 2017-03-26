#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

. $DIR/params.sh

PACKAGE_NAME="cairo-1.12.2"
FILE_NAME=$PACKAGE_NAME".tar.xz"
[ -f $TAR_DIR/$FILE_NAME ] || wget "http://cairographics.org/releases/$FILE_NAME" -O $TAR_DIR/$FILE_NAME
if [ ! -d $SRC_DIR/$PACKAGE_NAME ]; then
    pushd $SRC_DIR
    tar -xf $TAR_DIR/$FILE_NAME -C $SRC_DIR
    patch -p1 < $DIR/patches/$PACKAGE_NAME.patch
    popd
fi

[ -d $BUILD_DIR/$PACKAGE_NAME ] || mkdir -p $BUILD_DIR/$PACKAGE_NAME
pushd $BUILD_DIR/$PACKAGE_NAME
export PKG_CONFIG_PATH=/var/moo/i386-pc-moo/lib/pkgconfig
# ugly hack
cp -r $SYSROOT/include/pixman-1/* $SYSROOT/include
cp -r $SYSROOT/include/freetype2/* $SYSROOT/include/
$SRC_DIR/$PACKAGE_NAME/configure --host=$TARGET --prefix=$PREFIX --enable-ps=no --enable-pdf=no --enable-interpreter=no --enable-xlib=no CC=i386-pc-moo-gcc
cp $DIR/patches/cairo-dummy-Makefile test/Makefile
cp $DIR/patches/cairo-dummy-Makefile perf/Makefile
echo -e "\n\n#define CAIRO_NO_MUTEX 1" >> config.h
make
make DESTDIR=$SYSROOT install
popd
