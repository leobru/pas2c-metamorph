#!/bin/sh
if [ $# -ne 1 -a $# -ne 2 ]; then
    echo "usage: $0 input-file [-d]" >&2
    exit 2
fi
rm -f lexsrc.bin tokens.bin
/bin/echo -ne '\000' >> lexsrc.utxt
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
*call pashelp
P 2 0 1000440000B .
*execute
EOF
sed 's/{/<:/g;s/}/:>/g' < "$1" | ./preprocess.py >> tmp$$
echo '*end file' >> tmp$$
ulimit -t 3
if [ "$2" = "-d" ]; then ln -f tmp$$ lexer.dub ; fi
dubna tmp$$ | tee lexer.lst
status=$?
rm -f tmp$$
exit $status
# sed 's/{/<:/g;s/}/:>/g' < "$1" | ./preprocess.py > lexsrc.utxt
