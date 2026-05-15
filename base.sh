#!/bin/sh
cat << EOF > tmp$$
*NAME base
*disc:1/local
*file:base,67,w
*NO LIST
*pascal
EOF
# Compiling the compiler expressed in "standard" BESM-6 Pascal
# using the existing system compiler
sed 's/{/_(/g;s/}/_)/g' < base.pas >> tmp$$
cat << EOF >> tmp$$
*     Overwriting the old state of the object area
*copy:20,270000,670000
*     Writing the new binary object to base.bin
*to perso:670000
*end file
EOF
# The compilation must finish within 3 seconds
ulimit -t 3
rm -f base.o
if [ "$1" = "-d" ]; then ln -f tmp$$ base.dub ; fi
length=`dubna tmp$$ | tee base.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
# The compilation must contain the source stats
grep -q 'LINES STRUCTURE 1' base.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
echo Module length is $length zones
dd bs=6k skip=2 count=$length < base.bin > base.o
dtran -d base.o > base.asm
rm -f tmp$$
