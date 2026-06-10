#!/bin/sh
rm -f wrksrc.bin
sed 's/{/<:/g;s/}/:>/g' < work.p2c > wrksrc.utxt
echo '                                                                                ' >> wrksrc.utxt
cat << EOF > tmp$$
*NAME work
*disc:1/local
*file:pascom,42
*file:libc,43
*file:self,41
*file:wrksrc,44
*file:self2,67,w
*system
*     *pascom and pasmitxt
*libra:42
*     taking the work compiler module
*libra:41
*libra:43
*libra:22
*call pashelp
P 2 0 1000440000B .
*call *pascom
*copy:20,270000,670000
*table:exclude(pascontr)
*exclude
*to perso:670000
*end file
EOF
if [ "$1" = "-d" ]; then ln -f tmp$$ self2.dub ; fi
ulimit -t 3
rm -f self2.o
length=`dubna tmp$$ | tee self2.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
grep -q 'LINES STRUCTURE 1' self2.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
echo Module length is $length zones
dd bs=6k skip=2 count=$length < self2.bin > self2.o
dtran -d self2.o > self2.asm
rm -f tmp$$
