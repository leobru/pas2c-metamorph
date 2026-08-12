#!/bin/sh
work_module=${WORK_MODULE:-work}
src=$1
input_file="${src%.p2c}.input"
rm -f tmpsrc.bin tmpsrc.txt
sed 's/{/<:/g;s/}/:>/g' < "$src" > tmpsrc.utxt
echo '                                                                                 ' >> tmpsrc.utxt
cat << EOF > tmp$$
*NAME work
*disc:1/local
*file:pascom,42
*file:libc,43
*file:$work_module,41
*file:tmpsrc,44
*     *pascom and pasmitxt
*libra:42
*perso:43,cont
*     taking the $work_module compiler module
*libra:41
*libra:22
*call pashelp
P 2 0 1000440000B .
*call *pascom
*copy:0,000000,000000
*no load list
*execute
EOF
if [ -f "$input_file" ]; then
    cat "$input_file" >> tmp$$
fi
cat << EOF >> tmp$$
*end file
EOF
if [ "$src" = "-d" ]; then ln -f tmp$$ run.dub ; fi
ulimit -t 3
dubna tmp$$ | tail -n +41 | tee runwork.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
rm -f tmp$$
