#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

. $DIR/params.sh

PACKAGE_NAME="libpng-1.5.28"
FILE_NAME=$PACKAGE_NAME".tar.gz"
[ -f $TAR_DIR/$FILE_NAME ] || wget "https://downloads.sourceforge.net/project/libpng/libpng15/1.5.28/libpng-1.5.28.tar.gz?r=&ts=1490194502&use_mirror=netcologne" -O $TAR_DIR/$FILE_NAME
if [ ! -d $SRC_DIR/$PACKAGE_NAME ]; then
    pushd $SRC_DIR
    tar -xf $TAR_DIR/$FILE_NAME -C $SRC_DIR
    patch -p1 < $DIR/patches/$PACKAGE_NAME.patch
    popd
fi

[ -d $BUILD_DIR/$PACKAGE_NAME ] || mkdir -p $BUILD_DIR/$PACKAGE_NAME
pushd $BUILD_DIR/$PACKAGE_NAME
$SRC_DIR/$PACKAGE_NAME/configure --host=$TARGET --prefix=$PREFIX
make
make DESTDIR=$SYSROOT install
popd
