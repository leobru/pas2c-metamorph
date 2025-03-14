#!/bin/sh
cat << EOF > tmp$$
ШИФР 419900ЗС5^
ЛЕН 41(1234)42(2148)67(1234-WR)^
EEB1A3
*NAME self
*     taking the compiled work.p2c module
*table:liblist(pascompl)
*liblist:410100
*      pascal runtime library
*tapes:420440
*libra:42
*call yesmemory
*     system
*call *pascom
EOF
# Compiling the compiler expressed in the syntax currently supported
# by the base compiler, by itself.
sed 's/{/_(/g;s/}/_)/g' < work.p2c >> tmp$$
cat << EOF >> tmp$$
*to perso:670200
*end file
\`\`\`\`\`\`
ЕКОНЕЦ
EOF
if [ "$1" = "-d" ]; then ln -f tmp$$ self.b6 ; fi
length=`dispak -l tmp$$ | tee self.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
echo Module length is $length zones
rm -f self.o
besmtool dump 1234 --start=0202 --length=$length --to-file=self.o
dtran -d self.o > self.asm
rm -f tmp$$
