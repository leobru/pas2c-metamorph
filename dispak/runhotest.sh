#!/bin/sh
# Compile one test with host ./base and run it under dispak.
set -e
. "$(dirname "$0")/env.sh"

if [ "$1" = "-d" ]; then
    debug=1
    shift
fi
if [ $# -ne 1 ]; then
    echo "usage: $0 [-d] test.p2c" >&2
    exit 2
fi

src=$1
input_file="${src%.p2c}.input"
call_file="${src%.p2c}.call"
job=tmp$$
trap 'rm -f "$job" "$VOL_WORK" tmpbin.bin tmpbin.raw.o tmpbin.o' EXIT

if [ ! -f libc.bin ]; then
    echo "$0: libc.bin is required" >&2
    exit 1
fi

sed 's/{/<:/g;s/}/:>/g' < "$src" > tmpbin.utxt
if ! ./base tmpbin.utxt tmpbin.o > runhotest.compile.lst; then
    cat runhotest.compile.lst
    echo '*EXECUTE'
    echo ' БЫЛИ OШИБKИ ПPИ BBOДE ИЛИ TPAHCЛЯЦИИ !!!'
    echo '------------------------------------------------------------'
    exit 0
fi
tail -c +7 tmpbin.o > tmpbin.raw.o
./reconstruct-bin-header.py wrap --zones 16 tmpbin.raw.o tmpbin.bin || exit 1

write_vol "$VOL_WORK" tmpbin.bin
write_vol "$VOL_LIBC" libc.bin

{
    passport_header \
        "dis 41(${VOL_WORK})43(${VOL_LIBC})^" \
        "dis 31(2113)32(77)33(2148-440)34(2048)^"
    cat << EOF
*name hotestc
*libra:13
*perso:41
*perso:43,cont
*no load list
EOF
    if [ -f "$call_file" ]; then
        cat "$call_file"
    else
        echo '*execute'
    fi
    if [ -f "$input_file" ]; then
        cat "$input_file"
    fi
    job_trailer
} > "$job"

if [ "$debug" = 1 ]; then
    ln -f "$job" runhotest.job
fi

( ulimit -t 3; exec dispak --input-encoding=utf8 -l "$job" ) > runhotest.lst
status=$?
cat runhotest.lst
rm -f "$job"

if [ "$status" -eq 137 ]; then
    echo '[1;31mCPU CAP[22;39m'
    exit 137
fi
if [ "$status" -ne 0 ]; then
    echo '[1;31mFAILURE[22;39m'
    exit 1
fi
