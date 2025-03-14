#!/bin/sh
# Compiling a small test program with the base compiler
# disassembling and running
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
sed 's/{/_(/g;s/}/_)/g' < $1 >> tmp$$
cat << EOF >> tmp$$
*libra:2
*call dtran(program)
*super
*edit
*r:1
*ll
*ee
*execute
*end file
\`\`\`\`\`\`
ЕКОНЕЦ
EOF
dispak -l tmp$$ 
rm -f tmp$$
