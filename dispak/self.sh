#!/bin/sh
# Self-host work.p2c under dispak: work.bin recompiles work.p2c → self.bin/.o
set -e
. "$(dirname "$0")/env.sh"

job=tmp$$
trap 'rm -f "$job" "$VOL_OUT" "$VOL_SRC" wrksrc.bin' EXIT

for f in work.bin ccom.bin libc.bin; do
    if [ ! -f "$f" ]; then
        echo "$0: $f is required" >&2
        exit 1
    fi
done

sed 's/{/<:/g;s/}/:>/g' < work.p2c | ./preprocess.py > wrksrc.utxt
zones=$(python3 "$DISPAK_DIR/utxt2bin.py" wrksrc.utxt wrksrc.bin)
# PASHELP LLLL440000 designator: zones in octal, lun 44.
src_extent=$(printf '%04o440000' "$zones")

write_vol "$VOL_CCOM" ccom.bin
write_vol "$VOL_LIBC" libc.bin
write_vol "$VOL_WORK" work.bin
write_vol "$VOL_SRC" wrksrc.bin

rm -f "$VOL_OUT"
touch "$VOL_OUT"
besmtool zero "$VOL_OUT" --length=40 >/dev/null

{
    passport_header \
        "dis 41(${VOL_WORK})42(${VOL_CCOM})43(${VOL_LIBC})^" \
        "dis 44(${VOL_SRC})67(${VOL_OUT}-wr)^" \
        "dis 31(2113)32(77)33(2148-440)34(2048)^"
    cat << EOF
*name work
*no list
*assem
 PASCONTR:,NAME,DTRAN  /01.06.84/
 PASINFOR:,LC,18
 C/SETEXF:,SUBP,
 RGEXPORT:,LC,1
 14,VTM,*0004B
 14,XTA,2
 ,UTC,RGEXPORT
 ,ATX,
 14,XTA,3
 ,UTC,PASINFOR
 ,ATX,3
 ,UJ,C/SETEXF
 *0004B:,TEXT,8HPASINPUT
 ,LOG,${src_extent} 400
 ,LOG,2
 ,INT,0
 ,END,
*system
*libra:42
*libra:41
*libra:43
*libra:13
*call allmemory
*call ccom
*copy:20,270000,670000
*table:exclude(pascontr)
*exclude
*to perso:670000
EOF
    job_trailer
} > "$job"

rm -f self.o self.bin
run_dispak 8 "$job" self.lst
grep -q 'LINES STRUCTURE 1' self.lst
length=$(grep 'HA LIBRARY' self.lst | cut -d ' ' -f 5)
length=$((0${length:-0} - 2))
echo "Module length is $length zones"
dump_vol "$VOL_OUT" self.bin --length="$length"
./reconstruct-bin-header.py extract self.bin self.o
dtran -d self.o > self.asm
