#!/bin/sh
# Build ccom.bin from build-ccom.dub under dispak.
set -e
. "$(dirname "$0")/env.sh"

job=tmp$$
trap 'rm -f "$job" "$VOL_OUT"' EXIT

rm -f "$VOL_OUT"
touch "$VOL_OUT"
besmtool zero "$VOL_OUT" --length=80 >/dev/null

{
    passport_header \
        "dis 67(${VOL_OUT}-wr)^" \
        "dis 31(2113)32(77)33(2148-440)34(2048)^"
    # Replace dubna local-file mounts with the passport LUN; keep the rest.
    sed -e '/^\*disc:/d' -e '/^\*file:ccom/d' -e '/^\*end file$/d' build-ccom.dub
    job_trailer
} > "$job"

run_dispak 60 "$job" ccom.lst
length=$(grep 'HA LIBRARY' ccom.lst | cut -d ' ' -f 5)
length=$((0${length:-0} - 2))
if [ "$length" -lt 1 ]; then
    echo '[1;31mNO WRITE[22;39m'
    rm -f ccom.bin
    exit 1
fi
echo "Module length is $length zones"
dump_vol "$VOL_OUT" ccom.bin --length="$length"
