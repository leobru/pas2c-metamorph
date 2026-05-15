#!/bin/sh
cat << EOF > tmp$$
*NAME work
*disc:1/local
*file:pascom,42
*file:base,41
*     *pascom and pasmitxt
*libra:42
*     taking the base compiler module
*libra:41
*libra:22
*call *pascom
EOF
sed 's/{/<:/g;s/}/:>/g' < $1 >> tmp$$
cat << EOF >> tmp$$
*copy:0,000000,000000
*libra:23
*call dtran(program)
real:17
*assem
*read:1
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
