#!/bin/sh
# Compiling a small test program with the base compiler
# disassembling and running
rm -f tmp.bin
sed 's/{/<:/g;s/}/:>/g' < $1 > tmp.utxt
cat << EOF > tmp$$
*NAME test
*disc:1/local
*file:pascom,42
*file:libc,43
*file:tmp,44
*file:base,41
*libra:22
*     runtime library
*libra:42
*     taking the base compiler module
*libra:41
*call pashelp
P 2 0 1000440000B .
*call *pascom
*libra:23
*call dtran(program)
*call setftn:one,long
*edit
*r:1
*ll
*ee
*libra:43
*execute
EOF
if [ "$2" != "" ]; then
cat $2 >> tmp$$
fi
cat << EOF >> tmp$$
*end file
EOF
ulimit -t 3
dubna tmp$$
rm -f tmp$$
