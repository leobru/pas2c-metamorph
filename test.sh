#!/bin/sh
# Compiling a small test program with the base compiler
# disassembling and running
cat << EOF > tmp$$
ШИФР 419900ЗС5^
ЛЕН 41(1234)42(2148)^
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
sed 's/{/<:/g;s/}/:>/g' < $1 >> tmp$$
cat << EOF >> tmp$$
*libra:2
*call dtran(main)
*super
*edit
*r:1
*ll
*ee
*execute
EOF
if [ "$2" != "" ]; then
cat $2 >> tmp$$
fi
cat << EOF >> tmp$$
*end file
\`\`\`\`\`\`
ЕКОНЕЦ
EOF
ulimit -t 3
dispak -l tmp$$ 
rm -f tmp$$
