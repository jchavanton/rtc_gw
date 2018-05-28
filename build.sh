#!/bin/sh
cd ../..
SRC_DIR=`pwd`
export PATH=$PATH:$SRC_DIR/../../depot_tools
echo $PATH
ninja -C out/Default
