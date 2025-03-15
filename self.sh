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
sed 's/{/<:/g;s/}/:>/g' < work.p2c >> tmp$$
cat << EOF >> tmp$$
*copy:20,270000,670200
*to perso:670200
*end file
\`\`\`\`\`\`
ЕКОНЕЦ
EOF
if [ "$1" = "-d" ]; then ln -f tmp$$ self.b6 ; fi
ulimit -t 3
rm -f self.o
length=`dispak -l tmp$$ | tee self.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
grep -q 'LINES STRUCTURE' self.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
echo Module length is $length zones
besmtool dump 1234 --start=0202 --length=$length --to-file=self.o
dtran -d self.o > self.asm
rm -f tmp$$
