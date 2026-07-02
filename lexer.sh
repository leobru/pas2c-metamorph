#!/bin/sh
if [ $# -ne 1 -a $# -ne 2 ]; then
    echo "usage: $0 input-file [-d]" >&2
    exit 2
fi
rm -f lexsrc.bin tokens.bin
cat << EOF > tmp$$
*NAME lexer
*disc:1/local
*file:lexsrc,44
*file:tokens,45,w
*NO LIST
*pascal
EOF
sed 's/{/_(/g;s/}/_)/g' < lexer.pas | ./preprocess.py >> tmp$$
cat << EOF >> tmp$$
*libra:22
*no lo
*execute
*end file
EOF
sed 's/{/<:/g;s/}/:>/g' < "$1" | ./preprocess.py > lexsrc.utxt
dd bs=120 count=1 < /dev/zero >> lexsrc.utxt
ulimit -t 5
if [ "$2" = "-d" ]; then ln -f tmp$$ lexer.dub ; fi
dubna tmp$$ | tee lexer.lst
status=$?
rm -f tmp$$
exit $status
# sed 's/{/<:/g;s/}/:>/g' < "$1" | ./preprocess.py > lexsrc.utxt
