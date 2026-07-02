#!/bin/sh
runner=base
if [ "$1" = "-work" ]; then
    runner=work
    shift
fi
tokens="${1:-tokens.bin}"
if [ ! -f "$tokens" ]; then
    echo "usage: $0 [-work] [tokens-file]" >&2
    echo "$0: $tokens not found (run lexer.sh first)" >&2
    exit 1
fi
rm -f lexdump.bin
cat << EOF > tmp$$
*NAME lexdump
*disc:1/local
*file:pascom,42
*file:libc,43
*file:lexsrc,44
*file:tokens,45,r
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
sed 's/{/<:/g;s/}/:>/g' < lexdump.p2c > lexsrc.utxt
dd bs=120 count=1 < /dev/zero >> lexsrc.utxt
cat << EOF >> tmp$$
*copy:0,000000,000000
*      libra:23
*      call dtran(program)
*      call setftn:one,long
*      assem
*      read:1
*perso:43,cont
*no load list
*execute
*end file
EOF
if [ "$tokens" != "tokens.bin" ]; then
    cp "$tokens" tokens.bin
fi
ulimit -t 5
dubna tmp$$ | tee lexdumpp2c.lst
status=$?
rm -f tmp$$
exit $status
