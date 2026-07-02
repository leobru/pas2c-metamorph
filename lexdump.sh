#!/bin/sh
if [ $# -gt 1 ]; then
    echo "usage: $0 [tokens-file]" >&2
    exit 2
fi
tokens="${1:-tokens.bin}"
if [ ! -f "$tokens" ]; then
    echo "$0: $tokens not found (run lexer.sh first)" >&2
    exit 1
fi
rm -f lexdump.bin
cat << EOF > tmp$$
*NAME lexdump
*disc:1/local
*file:tokens,45,r
*NO LIST
*pascal
EOF
sed 's/{/_(/g;s/}/_)/g' < lexdump.pas | ./preprocess.py >> tmp$$
cat << EOF >> tmp$$
*libra:22
*no lo
*execute
*end file
EOF
ulimit -t 5
if [ "$tokens" != "tokens.bin" ]; then
    cp "$tokens" tokens.bin
fi
dubna tmp$$ | tee lexdump.lst
status=$?
rm -f tmp$$
exit $status
