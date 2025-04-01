#!/bin/sh
# Compiling a small test program with the base compiler
# disassembling and running
cat << EOF > tmp$$
*NAME test
*disc:1/local
*file:pascom,42
*file:libc,43
*file:base,41
*     runtime library
*libra:42
*     taking the base compiler module
*libra:41
*call *pascom
EOF
# Compiling a test case expressed in the syntax currently supported
# by the base compiler.
sed 's/{/<:/g;s/}/:>/g' < $1 >> tmp$$
cat << EOF >> tmp$$
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
