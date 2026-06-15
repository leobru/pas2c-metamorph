#!/bin/sh
cat << EOF > tmp$$
*NAME libc
*disc:1/local
*file:libc,67,w
*     NO LIST
*assem
EOF
if [ "$1" = "-d" ]; then ln -f tmp$$ libc.dub ; shift; fi
cat libc/*.madlen >> tmp$$
cat << EOF >> tmp$$
*call tcatalog
*to perso:670000
*end file
EOF
length=`dubna tmp$$ | tee libc.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$((0$length-2))
if [ $? -ne 0 ]; then
    echo '[1;31mFAILURE[22;39m'
    rm -f libc.bin
exit 1
fi
if [ $length -lt 1 ]; then
    echo '[1;31mNO WRITE[22;39m'
    rm -f libc.bin
exit 1
fi

echo Module length is $length zones
rm -f tmp$$
