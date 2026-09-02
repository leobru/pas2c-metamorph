#!/bin/sh
# Per-test runner for runhotests.sh: compile one test with the host-native
# ./base and run it under the DUBNA emulator, capturing the output.
if [ "$1" = "-d" ]; then
    debug=1
    shift
fi
if [ $# -ne 1 ]; then
    echo "usage: $0 [-d] test.p2c" >&2
    exit 2
fi

src="$1"
input_file="${src%.p2c}.input"
lun=41

rm -f tmpbin.bin tmpbin.txt tmpbin.o tmpbin.raw.o tmpbin.bin
sed 's/{/<:/g;s/}/:>/g' < "$src" > tmpbin.utxt

if ! ./base tmpbin.utxt tmpbin.o > runhotest.compile.lst; then
    cat runhotest.compile.lst
    echo '*EXECUTE'
    echo ' БЫЛИ OШИБKИ ПPИ BBOДE ИЛИ TPAHCЛЯЦИИ !!!'
    echo '------------------------------------------------------------'
    exit 0
fi
tail -c +7 tmpbin.o > tmpbin.raw.o
./reconstruct-bin-header.py wrap --zones 16 tmpbin.raw.o tmpbin.bin || exit 1

cat << EOF > tmp$$
*NAME hotestc
*disc:1/local
*file:tmpbin,$lun
*file:libc,43
*libra:22
*perso:$lun
*perso:43,cont
*no load list
*execute
EOF
if [ -f "$input_file" ]; then
    cat "$input_file" >> tmp$$
fi
cat << EOF >> tmp$$
*end file
EOF
if [ "$debug" = 1 ]; then ln -f tmp$$ runhotest.dub ; fi
# Run through a file rather than a pipe: /bin/sh has no PIPESTATUS, so with
# `dubna | tee` the status inspected below is tee's and dubna's is lost --
# which is how a timed-out emulator used to look like a clean run.
( ulimit -t 3; exec dubna tmp$$ ) > runhotest.lst
status=$?
cat runhotest.lst
if [ $status -eq 137 ]; then
echo '[1;31mCPU CAP[22;39m'
rm -f tmp$$
exit 137                # runhotests.sh reports 137 as an infinite loop
fi
if [ $status -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
rm -f tmp$$
