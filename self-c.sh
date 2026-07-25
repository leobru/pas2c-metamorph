#!/bin/sh
# Self-host work-c.p2c under the DUBNA emulator: run the host-built workc.bin
# compiler module against a preprocessed copy of its own source, producing
# selfc.bin/.o for comparison against the host-built workc.o (check.sh).
# Mirrors self.sh for the C-declarator experimental fork.
rm -f wcsrc.bin
sed 's/{/<:/g;s/}/:>/g' < work-c.p2c | ./preprocess.py > wcsrc.utxt
echo '                                                                                ' >> wcsrc.utxt
cat << EOF > tmp$$
*NAME workc
*disc:1/local
*file:pascom,42
*file:libc,43
*file:workc,41
*file:wcsrc,44
*file:selfc,67,w
*system
*     *pascom and pasmitxt
*libra:42
*     taking the work compiler module
*libra:41
*libra:43
*libra:22
*call pashelp
P 2 0 1000440000B .
*call *pascom
*copy:20,270000,670000
*table:exclude(pascontr)
*exclude
*to perso:670000
*end file
EOF
if [ "$1" = "-d" ]; then ln -f tmp$$ self-c.dub ; fi
ulimit -t 3
rm -f selfc.o
length=`dubna tmp$$ | tee selfc.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
grep -q 'LINES STRUCTURE 1' selfc.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
echo Module length is $length zones
# Extract the exact-length object (matches the host-built workc.o; a plain
# zone-granular dd would leave trailing padding and break the fixpoint cmp).
./reconstruct-bin-header.py extract selfc.bin selfc.o
dtran -d selfc.o > selfc.asm
rm -f tmp$$
