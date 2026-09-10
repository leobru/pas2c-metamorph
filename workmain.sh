#!/bin/sh
# Compile workmain.p2c (the tiny main-bearing half of the split compiler)
# with the host-native compiler (base, from base.cc) into the raw object
# wmain.o and the emulator-loadable module wmain.bin.
#
# The artifacts are named "wmain", not "workmain": every DUBNA *file: card
# name is at most six characters, and "workmain" is eight. wmain.bin is
# what every consumer script attaches directly - no script needs its own
# short-name copy of it.
#
# This half never needs real DUBNA cataloging: it holds MAIN, so it only
# ever has to serve as the module a loader jumps into by its start word,
# never as something another module resolves an extern call INTO by name -
# see separate_main_plan.md's Step 0 spike for why that distinction is what
# decides which half needs which build path. work.sh (the library half)
# is the one that needs the real catalog step.
rm -f wmain.bin
sed 's/{/<:/g;s/}/:>/g' < workmain.p2c | ./preprocess.py > wmsrc.utxt
rm -f wmain.raw.o wmain.o wmain.bin
./base wmsrc.utxt wmain.tmp.o > wmain.lst
grep -q 'LINES STRUCTURE 1' wmain.lst
if [ $? -ne 0 ]; then
printf '\033[1;31mFAILURE\033[22;39m\n'
grep -A 2 '\*\*\*[1-9]' wmain.lst
exit 1
fi
tail -c +7 wmain.tmp.o > wmain.raw.o
cp wmain.raw.o wmain.o
./reconstruct-bin-header.py wrap --zones 16 wmain.raw.o wmain.bin || exit 1
dtran -d wmain.o > wmain.asm
rm -f wmain.tmp.o
