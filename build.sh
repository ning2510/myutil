#!/bin/bash

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-./build}
BIN_DIR=${BIN_DIR:-./bin}
LIB_DIR=${LIB_DIR:-./lib}

mkdir -p ${BUILD_DIR} \
    && mkdir -p ${LIB_DIR} \
    && mkdir -p ${BIN_DIR} \
    && cd ${BUILD_DIR} \
    && cmake .. \
    && make install \
    && cd .. \