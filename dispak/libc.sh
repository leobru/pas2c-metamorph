#!/bin/sh
# Assemble libc/*.madlen into libc.bin under dispak.
set -e
. "$(dirname "$0")/env.sh"

job=tmp$$
trap 'rm -f "$job" "$VOL_OUT"' EXIT

rm -f "$VOL_OUT"
touch "$VOL_OUT"
besmtool zero "$VOL_OUT" --length=40 >/dev/null

{
    passport_header \
        "dis 67(${VOL_OUT}-wr)^" \
        "dis 31(2113)32(77)33(2148-440)34(2048)^"
    cat << EOF
*name libc
*call setftn:one,long
*assem
EOF
    cat libc/*.madlen
    cat << EOF
*call tcatalog
*to perso:670000
EOF
    job_trailer
} > "$job"

run_dispak 30 "$job" libc.lst
length=$(grep 'HA LIBRARY' libc.lst | cut -d ' ' -f 5)
length=$((0${length:-0} - 2))
if [ "$length" -lt 1 ]; then
    echo '[1;31mNO WRITE[22;39m'
    rm -f libc.bin
    exit 1
fi
echo "Module length is $length zones"
dump_vol "$VOL_OUT" libc.bin --length="$length"
