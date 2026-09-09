#!/bin/sh
# Compile and run one .p2c test through the work compiler under dispak.
set -e
. "$(dirname "$0")/env.sh"

work_module=${WORK_MODULE:-work}
src=$1
if [ -z "$src" ]; then
    echo "usage: $0 test.p2c" >&2
    exit 2
fi
input_file="${src%.p2c}.input"
call_file="${src%.p2c}.call"

job=tmp$$
trap 'rm -f "$job" "$VOL_SRC" tmpsrc.bin' EXIT

for f in "${work_module}.bin" ccom.bin libc.bin; do
    if [ ! -f "$f" ]; then
        echo "$0: $f is required" >&2
        exit 1
    fi
done

sed 's/{/<:/g;s/}/:>/g' < "$src" > tmpsrc.utxt
zones=$(python3 "$DISPAK_DIR/utxt2bin.py" tmpsrc.utxt tmpsrc.bin)
src_extent=$(printf '%04o440000' "$zones")

write_vol "$VOL_CCOM" ccom.bin
write_vol "$VOL_LIBC" libc.bin
write_vol "$VOL_WORK" "${work_module}.bin"
write_vol "$VOL_SRC" tmpsrc.bin

{
    passport_header \
        "dis 41(${VOL_WORK})42(${VOL_CCOM})43(${VOL_LIBC})^" \
        "dis 44(${VOL_SRC})^" \
        "dis 31(2113)32(77)33(2148-440)34(2048)^"
    cat << EOF
*name work
*libra:42
*perso:43,cont
*libra:41
*libra:13
*call pashelp
P 2 0 ${src_extent}B .
*call ccom
*copy:0,000000,000000
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

raw=tmpraw$$
( ulimit -t 3; exec dispak --input-encoding=utf8 -l "$job" ) > "$raw"
status=$?
# Drop the Monitor-80 banner; keep from the first *NAME / *CALL onward when present.
awk '
  /^\*NAME/ || /^\*CALL/ || /^\*EXECUTE/ { show=1 }
  show { print }
  END { if (!show) print }
' "$raw" | tee runwork.lst
rm -f "$raw" "$job"

if [ "$status" -eq 137 ]; then
    echo '[1;31mCPU CAP[22;39m'
    exit 137
fi
if [ "$status" -ne 0 ]; then
    echo '[1;31mFAILURE[22;39m'
    exit 1
fi
