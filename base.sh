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
length=`dispak -l tmp$$ | tee base.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
echo Module length is $length zones
rm -f base.o
besmtool dump 1234 --start=2 --length=$length --to-file=base.o
dtran -d base.o > base.asm
if [ "$1" = "-d" ]; then mv tmp$$ base.b6 ; else rm -f tmp$$; fi
