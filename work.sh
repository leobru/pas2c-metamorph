#!/bin/sh
cat << EOF > tmp$$
*NAME work
*disc:1/local
*file:pascom,42
*file:base,41
*file:work,67,w
*     *pascom and pasmitxt
*libra:42
*     taking the base compiler module
*libra:41
*call *pascom
EOF
# Compiling the compiler expressed in the syntax currently supported
# by the base compiler.
sed 's/^$/ /;s/{/<:/g;s/}/:>/g' < work.p2c >> tmp$$
cat << EOF >> tmp$$
*copy:20,270000,670000
*to perso:670000
*end file
EOF
ulimit -t 5
rm -f work.o
if [ "$1" = "-d" ]; then ln -f tmp$$ work.dub ; fi
length=`dubna tmp$$ | tee work.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
grep -q 'LINES STRUCTURE 1' work.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
echo Module length is $length zones
dd bs=6k skip=2 count=$length < work.bin > work.o
dtran -d work.o > work.asm
rm -f tmp$$
