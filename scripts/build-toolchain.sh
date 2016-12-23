#!/bin/bash
PREFIX="/var/moo/"
TARGET="i386-pc-moo"
SYS_ROOT="/var/moo/sysroot/"
<<"COMMENT"
mkdir ./packages
wget "http://ftp.gnu.org/gnu/binutils/binutils-2.22.tar.gz" -O ./packages/binutils-2.22.tar.gz
mkdir ./src
tar -xf ./packages/binutils-2.22.tar.gz -C ./src
patch -c -p3 -d./src/binutils-2.22 < ./patches/binutils-2.22.patch
cp -r ./patches/binutils-2.22/* ./src/binutils-2.22/
mkdir ./build
cd ./build
mkdir ./binutils
cd ./binutils
../../src/binutils-2.22/configure --target=$TARGET --prefix=$PREFIX --with-sysroot=$SYS_ROOT --disable-nls --disable-werror
make all
make install

mkdir ./packages
wget "http://www.netgull.com/gcc/releases/gcc-4.6.4/gcc-4.6.4.tar.gz" -O ./packages/gcc-4.6.4.tar.gz
mkdir ./src
tar -xf ./packages/gcc-4.6.4.tar.gz -C ./src
patch -c -p2 -d./src/gcc-4.6.4 < ./patches/gcc-4.6.4.patch
cp -r ./patches/gcc-4.6.4/* ./src/gcc-4.6.4/
mkdir ./build
cd ./build
mkdir ./gcc
cd ./gcc
../../src/gcc-4.6.4/configure --target=$TARGET --prefix=$PREFIX --with-sysroot=$SYS_ROOT --disable-nls --enable-languages=c
make all-gcc
make install-gcc
make all-target-libgcc
make install-target-libgcc
COMMENT
sudo apt-get install autoconf2.64
mkdir ./packages
#wget "ftp://sourceware.org/pub/newlib/newlib-1.19.0.tar.gz" -O ./packages/newlib-1.19.0.tar.gz
mkdir ./src
tar -xf ./packages/newlib-1.19.0.tar.gz -C ./src
patch -c -p3 -d./src/newlib-1.19.0 < ./patches/newlib-1.19.0.patch
cp -r ./patches/newlib-1.19.0/* ./src/newlib-1.19.0/
pushd ./src/newlib-1.19.0/newlib/libc/sys/moo
autoreconf2.64
cd ..
autoconf2.64
popd

mkdir ./build
cd ./build
mkdir ./newlib
cd ./newlib
#../../src/newlib-1.19.0/configure --target=$TARGET CFLAGS='-O0 -ggdb3' CXXFLAGS='-O0 -ggdb3'
#make all
#make DESTDIR=$PREFIX install
