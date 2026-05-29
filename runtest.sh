#!/bin/sh
rm -f tmpsrc.bin tmpsrc.txt
sed 's/{/<:/g;s/}/:>/g' < $1 > tmpsrc.utxt
echo '                                                                                 ' >> tmpsrc.utxt
cat << EOF > tmp$$
*NAME work
*disc:1/local
*file:pascom,42
*file:libc,43
*file:base,41
*file:tmpsrc,44
*     *pascom and pasmitxt
*libra:42
*     taking the base compiler module
*libra:41
*libra:22
*call pashelp
P 2 0 1000440000B .
*call *pascom
*copy:0,000000,000000
*no load list
*libra:43
*execute
*end file
EOF
if [ "$1" = "-d" ]; then ln -f tmp$$ run.dub ; fi
ulimit -t 3
dubna tmp$$ | tee runbase.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
rm -f tmp$$
