#!/bin/sh
cat << EOF > tmp$$
ШИФР 419900ЗС5^
ЛЕН 41(1234)42(2148)67(1234-WR)^
EEB1A3
*NAME base
*NO LIST
*call yesmemory
*     system
*     call *pascom
*pascal
EOF
# Compiling the compiler expressed in "standard" BESM-6 Pascal
# using the existing system compiler
sed 's/{/_(/g;s/}/_)/g' < base.pas >> tmp$$
cat << EOF >> tmp$$
*copy:20,270000,670000
*to perso:670000
*end file
\`\`\`\`\`\`
ЕКОНЕЦ
EOF
ulimit -t 3
rm -f base.o
if [ "$1" = "-d" ]; then ln -f tmp$$ base.b6 ; fi
length=`dispak -l tmp$$ | tee base.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
grep -q 'LINES STRUCTURE 1' base.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
echo Module length is $length zones
besmtool dump 1234 --start=2 --length=$length --to-file=base.o
dtran -d base.o > base.asm
rm -f tmp$$
