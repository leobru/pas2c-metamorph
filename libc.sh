#!/bin/sh
cat << EOF > tmp$$
*NAME libc
*disc:1/local
*file:libc,67,w
*NO LIST
EOF
cat libc.src >> tmp$$
cat << EOF >> tmp$$
*to perso:670000
*end file
EOF
length=`dubna tmp$$ | tee libc.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
echo Module length is $length zones
rm -f tmp$$
