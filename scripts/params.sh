#!/bin/bash
BUILD_DIR=~/moo_tools/build
SRC_DIR=~/moo_tools/src
TAR_DIR=~/moo_tools/tars

TARGET=i386-pc-moo
PREFIX=
SYSROOT=/var/moo/i386-pc-moo

[ -d $BUILD_DIR ] || mkdir -p $BUILD_DIR
[ -d $SRC_DIR ] || mkdir -p $SRC_DIR
[ -d $TAR_DIR ] || mkdir -p $TAR_DIR
