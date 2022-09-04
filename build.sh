#!/bin/bash

#
# Copyright (c) 2022 Christopher Gassib
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

game_name=Skeleton
arm_toolchain_dir=$PLAYDATE_SDK_PATH/../arm-gnu-toolchain-11.3.rel1-darwin-x86_64-arm-none-eabi/bin/

rm -rf "$game_name.pdx"
rm Source/pdex.bin
rm Source/pdex.dylib

# clean makefile cruft
#rm -rf build
#make device

if [ ! -d extern ]; then
  mkdir extern
  cd extern
  if [ ! -d clg-math ]; then
    git clone https://github.com/ChrisG0x20/clg-math
  fi
  cd -
fi

# make the simulator build directory
if [ ! -d sim_build ]; then
  mkdir sim_build
fi

# make the device build directory
if [ ! -d dev_build ]; then
  mkdir dev_build
fi

# build the simulator executable
cd sim_build
#cmake -G "Xcode" -DCMAKE_BUILD_TYPE=Debug ..
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
cd -

# build the device executable
cd dev_build
cmake -DCMAKE_TOOLCHAIN_FILE=arm.cmake -DARM_TOOLCHAIN_PATH=$arm_toolchain_dir -DCMAKE_BUILD_TYPE=Release ..
make
cd -

# build the PDX directory
pdc Source "$game_name"
