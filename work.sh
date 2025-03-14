#!/bin/sh
cat << EOF > tmp$$
ШИФР 419900ЗС5^
ЛЕН 41(1234)42(2148)67(1234-WR)^
EEB1A3
*NAME work
*      pascal runtime library
*tapes:420440
*libra:42
*     taking the base compiler module
*libra:41
*call yesmemory
*system
*call *pascom
EOF
# Compiling the compiler expressed in the syntax currently supported
# by the base compiler.
sed 's/{/_(/g;s/}/_)/g' < work.p2c >> tmp$$
cat << EOF >> tmp$$
*copy:20,270000,670100
*to perso:670100
*end file
\`\`\`\`\`\`
ЕКОНЕЦ
EOF
length=`dispak -l tmp$$ | tee work.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
echo Module length is $length zones
rm -f work.o
besmtool dump 1234 --start=0102 --length=$length --to-file=work.o
dtran -d work.o > work.asm
rm -f tmp$$
