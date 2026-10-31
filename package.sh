#!/bin/bash
# Stage System/Apps/Umoria for a CF card:  SDK=... ./package.sh <outdir>
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
SDK=${SDK:-$HOME/psion_cpp_sdk_linux}
OUT=${1:-$HERE/dist}
REL=$SDK/epoc_cpp_sdk/epoc32/release/marm/rel
A=$OUT/System/Apps/Umoria
rm -rf "$OUT" && mkdir -p "$A/data"
cp $REL/umoria.exe "$A/"
cp "$HERE/launcher/umoria.app" "$HERE/launcher/umoria.rsc" "$A/"
cp "$HERE/icon/umoria.aif" "$A/"
for f in "$HERE"/data/*; do
  b=$(basename "$f")
  case "$b" in
    *.in) sed -e 's/${umoria_version}/5.7.15/g' -e "s/${current_date}/$(date +%Y-%m-%d)/g" "$f" > "$A/data/${b%.in}" ;;
    scores.dat) ;;
    *) cp "$f" "$A/data/" ;;
  esac
done
: > "$A/scores.dat"
cp "$HERE/LICENSE" "$A/LICENSE"
ls -la "$A" "$A/data"
