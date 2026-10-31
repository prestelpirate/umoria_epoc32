#!/bin/bash
# Build umoria for EPOC32 (MARM release) using psion_cpp_sdk_linux.
#   SDK=/path/to/psion_cpp_sdk_linux [CPU=strongarm] [TRACE=1] ./build.sh
set -e
SDK=${SDK:-$HOME/psion_cpp_sdk_linux}
HERE=$(cd "$(dirname "$0")" && pwd)
cd $SDK && source ./start_epoc_sdk.sh
ln -sfn "$HERE" "$EPOCROOT/umoria"
cd "$HERE/group"
makmake umoria marm 2>&1 | grep -v 'may be incomplete' || true
M=umoria.marm
# C files: C99
sed -i '/\.c$/s/\$(GCC\(U\?\)\(REL\|DEB\))/$(GCC\1\2) -std=gnu99/' $M
# game C++ files: forced C++98 prelude, no exceptions/RTTI (see cxxflags.sed)
sed "s|PRELUDE|$HERE/compat/prelude.h|" cxxflags.sed > cxxflags.local.sed
sed -i -f cxxflags.local.sed $M
if [ "$CPU" = strongarm ]; then
  sed -i 's/-mcpu=arm710/-mcpu=strongarm110/; s/-mshort-load-bytes//' $M
fi
if [ "$TRACE" = 1 ]; then
  sed -i '/\/src\/[^ ]*\.cpp$/s/-fno-rtti/-fno-rtti -finstrument-functions/' $M
fi
make ${MAKEFLAGS_EXTRA} -f $M rel
