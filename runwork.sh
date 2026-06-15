#!/bin/sh
cat << EOF > tmp$$
*NAME work
*disc:1/local
*file:pascom,42
*file:work,41
*file:libc,43
*     *pascom and pasmitxt
*libra:42
*     taking the work compiler module
*libra:41
*perso:43,cont
*libra:22
*call *pascom
EOF
if [ "$1" = "-d" ]; then ln -f tmp$$ run.dub ; shift; fi
sed 's/{/<:/g;s/}/:>/g' < $1 >> tmp$$
cat << EOF >> tmp$$
*copy:0,000000,000000
*libra:23
*call dtran(program)
*call setftn:one,long
*assem
*read:1
*perso:43,cont
*no lo
*execute
*end file
EOF
ulimit -t 3
dubna tmp$$ | sed 1,/METAMORPH/d | tee runwork.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
rm -f tmp$$
