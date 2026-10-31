#!/bin/bash
# Build launcher .app files: build.sh name uid3 [name uid3 ...]
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
SDK=${SDK:-$HOME/psion_cpp_sdk_linux}
cd $SDK && source ./start_epoc_sdk.sh
ln -sfn $HERE $EPOCROOT/exelaunch
cd $HERE/group
while [ $# -ge 2 ]; do
  n=$1; u=$2; shift 2
  cp ../src/exelaunch.rss ../src/$n.rss
  sed "s/@NAME@/$n/g; s/@UID@/$u/" template.mmp > $n.mmp
  makmake $n marm >/dev/null 2>&1
  sed -i "s/^\(GCCDEFS *=.*\)$/\1 -DUID3=$u/" $n.marm
  make -f $n.marm clean >/dev/null 2>&1 || true
  make -f $n.marm rel > $n.log 2>&1 || { grep -v warning $n.log | grep -i -A2 error; exit 1; }
  mkdir -p $HERE/dist && cp $EPOCROOT/epoc32/release/marm/rel/$n.app $EPOCROOT/epoc32/release/marm/rel/$n.rsc $HERE/dist/
done
ls -la $HERE/dist
