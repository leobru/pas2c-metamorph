#!/bin/sh
# Like runhotest.sh, but drives the experimental C-declarator-only
# base-c binary instead of base. See the-idea-is-to-kind-naur plan.
if [ "$1" = "-d" ]; then
    debug=1
    shift
fi
if [ $# -ne 1 ]; then
    echo "usage: $0 [-d] test.p2c" >&2
    exit 2
fi

src="$1"
lun=41

rm -f tbinc.bin tbinc.txt tbinc.o tbinc.raw.o tbinc.bin
sed 's/{/<:/g;s/}/:>/g' < "$src" > tbinc.utxt
echo '                                                                                 ' >> tbinc.utxt

if ! ./base-c tbinc.utxt tbinc.o > run-base-c-test.compile.lst; then
    cat run-base-c-test.compile.lst
    echo '*EXECUTE'
    echo ' БЫЛИ OШИБKИ ПPИ BBOДE ИЛИ TPAHCЛЯЦИИ !!!'
    echo '------------------------------------------------------------'
    exit 0
fi
tail -c +7 tbinc.o > tbinc.raw.o
./reconstruct-bin-header.py wrap --zones 16 tbinc.raw.o tbinc.bin || exit 1

cat << EOF > tmp$$
*NAME hotestc
*disc:1/local
*file:tbinc,$lun
*file:libc,43
*libra:22
*perso:$lun
*perso:43,cont
*no load list
*execute
*end file
EOF
if [ "$debug" = 1 ]; then ln -f tmp$$ run-base-c-test.dub ; fi
ulimit -t 3
dubna tmp$$ | tee run-base-c-test.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
rm -f tmp$$
