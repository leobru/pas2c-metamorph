#!/bin/sh
rm -f wrksrc.bin
sed 's/{/<:/g;s/}/:>/g' < work.p2c | ./preprocess.py > wrksrc.utxt
echo '                                                                                ' >> wrksrc.utxt
cat << EOF > tmphoself
*NAME work
*disc:1/local
*file:pascom,42
*file:libc,43
*file:howork,41
*file:wrksrc,44
*file:hoself,67,w
*system
*     *pascom and pasmitxt
*libra:42
*     taking the host-built work compiler module
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
if [ "$1" = "-d" ]; then ln -f tmphoself hoself.dub ; fi
ulimit -t 3
rm -f hoself.o
length=`dubna tmphoself | tee hoself.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
grep -q 'LINES STRUCTURE 1' hoself.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
echo Module length is $length zones
./reconstruct-bin-header.py extract hoself.bin hoself.o
dtran -d hoself.o > hoself.asm
rm -f tmphoself
