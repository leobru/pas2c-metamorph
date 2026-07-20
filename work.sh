#!/bin/sh
# Compile work.p2c with the host-native compiler (base, from base.cc) into
# the raw object work.o and the emulator-loadable library module work.bin.
# This replaces the retired emulator base-module path (base.pas is dead).
rm -f wrksrc.bin
sed 's/{/<:/g;s/}/:>/g' < work.p2c | ./preprocess.py > wrksrc.utxt
echo '                                                                                ' >> wrksrc.utxt
rm -f work.raw.o work.o work.bin
./base wrksrc.utxt work.tmp.o > work.lst
grep -q 'LINES STRUCTURE 1' work.lst
if [ $? -ne 0 ]; then
printf '\033[1;31mFAILURE\033[22;39m\n'
grep -A 2 '\*\*\*[1-9]' work.lst
exit 1
fi
tail -c +7 work.tmp.o > work.raw.o
cp work.raw.o work.o
./reconstruct-bin-header.py wrap --zones 16 work.raw.o work.bin || exit 1
dtran -d work.o > work.asm
rm -f work.tmp.o
