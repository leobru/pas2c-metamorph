#!/bin/sh
# Compile work-c.p2c with the host-native compiler (base-c, from base-c.cc)
# into the raw object workc.o and the emulator-loadable library module
# workc.bin. Mirrors work.sh for the C-declarator experimental fork.
rm -f workc.bin
sed 's/{/<:/g;s/}/:>/g' < work-c.p2c | ./preprocess.py > wcsrc.utxt
echo '                                                                                ' >> wcsrc.utxt
rm -f workc.raw.o workc.o workc.bin
./base-c wcsrc.utxt workc.tmp.o > workc.lst
grep -q 'LINES STRUCTURE 1' workc.lst
if [ $? -ne 0 ]; then
printf '\033[1;31mFAILURE\033[22;39m\n'
grep -A 2 '\*\*\*[1-9]' workc.lst
exit 1
fi
tail -c +7 workc.tmp.o > workc.raw.o
cp workc.raw.o workc.o
./reconstruct-bin-header.py wrap --zones 16 workc.raw.o workc.bin || exit 1
dtran -d workc.o > workc.asm
rm -f workc.tmp.o
