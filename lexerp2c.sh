#!/bin/sh
rm -f lexsrc.bin
runner=base
if [ "$1" = "-work" ]; then
    runner=work
    shift
fi
if [ $# -ne 1 ]; then
    echo "usage: $0 [-work] input-file" >&2
    exit 2
fi
cat << EOF > tmp$$
*NAME lexer
*disc:1/local
*file:pascom,42
*file:libc,43
*file:lexsrc,44
EOF
if [ "$runner" = work ]; then
cat << EOF >> tmp$$
*file:work,41
*libra:42
*libra:41
EOF
else
cat << EOF >> tmp$$
*file:base,41
*libra:42
*libra:41
EOF
fi
cat << EOF >> tmp$$
*perso:43,cont
*libra:22
*call pashelp
P 2 0 1000440000B .
*call *pascom
EOF
sed 's/{/<:/g;s/}/:>/g' < lexer.p2c > lexsrc.utxt
printf '                                                                                \n' >> lexsrc.utxt
cat << EOF >> tmp$$
*copy:0,000000,000000
*      libra:23
*      call dtran(program)
*      call setftn:one,long
*      assem
*      read:1
*perso:43,cont
*      no load list
*execute
EOF
cat "$1" >> tmp$$
echo '*end file' >> tmp$$
ulimit -t 3
dubna tmp$$ | tee lexerp2c.lst
status=$?
rm -f tmp$$
exit $status
