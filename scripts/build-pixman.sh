#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

. $DIR/params.sh

PACKAGE_NAME="pixman-0.26.2"
FILE_NAME=$PACKAGE_NAME".tar.gz"
[ -f $TAR_DIR/$FILE_NAME ] || wget "https://www.cairographics.org/releases/$FILE_NAME" -O $TAR_DIR/$FILE_NAME
if [ ! -d $SRC_DIR/$PACKAGE_NAME ]; then
    pushd $SRC_DIR
    tar -xf $TAR_DIR/$FILE_NAME -C $SRC_DIR
    patch -p1 < $DIR/patches/$PACKAGE_NAME.patch
    popd
fi

[ -d $BUILD_DIR/$PACKAGE_NAME ] || mkdir -p $BUILD_DIR/$PACKAGE_NAME
pushd $BUILD_DIR/$PACKAGE_NAME
$SRC_DIR/$PACKAGE_NAME/configure --host=$TARGET --prefix=$PREFIX
#should be enabled after SSE / MMX support
echo -e "\n\n#undef USE_SSE2" >> config.h
echo -e "\n\n#undef USE_X86_MMX" >> config.h
echo -e "\n\n#undef TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR" >> config.h
make
make DESTDIR=$SYSROOT install
popd
