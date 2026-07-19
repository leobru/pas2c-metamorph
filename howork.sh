#!/bin/sh
rm -f wrksrc.bin
sed 's/{/<:/g;s/}/:>/g' < work.p2c | ./preprocess.py > wrksrc.utxt
echo '                                                                                ' >> wrksrc.utxt
rm -f howork.raw.o howork.o howork.bin
./base wrksrc.utxt howork.tmp.o > howork.lst 
grep -q 'LINES STRUCTURE 1' howork.lst
if [ $? -ne 0 ]; then
echo 'ESC[1;31mFAILUREESC[22;39m'
grep -A 2 '\*\*\*[1-9]' howork.lst
exit 1
fi
tail -c +7 howork.tmp.o > howork.raw.o
cp howork.raw.o howork.o
./reconstruct-bin-header.py wrap --zones 16 howork.raw.o howork.bin || exit 1
dtran -d howork.o > howork.asm
rm -f howork.tmp.o
