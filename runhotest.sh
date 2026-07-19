#!/bin/sh
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

rm -f tmpsrc.bin tmpsrc.txt tmpbin.o tmpbin.raw.o tmpbin.bin
sed 's/{/<:/g;s/}/:>/g' < "$src" > tmpsrc.utxt
echo '                                                                                 ' >> tmpsrc.utxt

if ! ./base tmpsrc.utxt tmpbin.o > runhotest.compile.lst; then
    cat runhotest.compile.lst
    echo '*EXECUTE'
    echo ' БЫЛИ OШИБKИ ПPИ BBOДE ИЛИ TPAHCЛЯЦИИ !!!'
    echo '------------------------------------------------------------'
    exit 0
fi
tail -c +7 tmpbin.o > tmpbin.raw.o
./reconstruct-bin-header.py wrap --zones 16 tmpbin.raw.o tmpbin.bin || exit 1

cat << EOF > tmp$$
*NAME hotest
*disc:1/local
*file:tmpbin,$lun
*file:libc,43
*libra:22
*perso:$lun
*perso:43,cont
*no load list
*execute
*end file
EOF
if [ "$debug" = 1 ]; then ln -f tmp$$ runhotest.dub ; fi
ulimit -t 3
dubna tmp$$ | tee runhotest.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
rm -f tmp$$
