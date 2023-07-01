#!/bin/sh
test -e mx3/mx3.h || git submodule update --init
#rm -rf build
mkdir build
cd build
# Note that gcc-mp-5 on darwin is still ~2000 MiB/sec faster than the system clang
# clang-mp-dev (4.0) also
case `uname -s` in
#Darwin)  CXX=clang++-mp-devel  CC=clang-mp-devel cmake .. ;;
#Darwin)  CXX=g++-mp-6  CC=gcc-mp-6 cmake .. ;;
Darwin)  CXX=clang++ CXXFLAGS='-O3 -std=c++14' cmake .. ;;
FreeBSD) CXX=clang++ CC=clang cmake .. ;;
Linux)   CXX=g++ CC=gcc cmake .. ;;
*)       CXX=clang++-4.0  CC=clang-4.0 cmake ..   ;;
esac
make -j4 $@
cp SMHasher ..
cd ..
