#!/bin/sh
# Compile work.p2c (the library half of the split compiler: everything
# except MAIN, which lives in workmain.p2c/workmain.sh, artifact wmain).
#
# work.o (the make-check fixpoint reference) and wlibo.bin (the
# name-resolvable library, staged for combine.sh -- not directly attached
# by any consumer script) come from one host `base` compile. base.cc's
# finalize() computes an entry-point table (name/offset pairs for each
# extern routine -- PROGRAMME, INITOPTIONS, INITTABLES, FINALIZE -- the
# loader has to resolve by name from another module) but never serializes
# it into the raw object: it only ever existed in memory, handed to the
# DUBNA runtime via pasinfor.entryptr for a *call tcatalog job to read
# directly (see work.p2c's own finalize). finalize() now also prints it
# (ENTRYCNT/ENTRYPT lines in work.lst), and
# reconstruct-bin-header.py wrap --entries reads that back to build the
# same real, name-searchable catalog directory a *call tcatalog job would
# -- reverse-engineered byte for byte against one, including the required
# all-ones end-of-table marker after the terminator.
#
# This needs no DUBNA compilation and no prior wlibo.bin/wmain.bin: base.cc
# alone is enough, closing the from-scratch bootstrap gap the old
# *call ccom/tcatalog pipeline had (see separate_main_plan.md's "Bootstrap
# regression"). combine.sh merges wlibo.bin with wmain.bin into the final
# work.bin every consumer script attaches, unchanged, at unit 41.
set -e
sed 's/{/<:/g;s/}/:>/g' < work.p2c | ./preprocess.py > wrksrc.utxt

rm -f work.raw.o work.o wlibo.bin
./base wrksrc.utxt work.tmp.o > work.lst
grep -q 'LINES STRUCTURE 1' work.lst
if [ $? -ne 0 ]; then
printf '\033[1;31mFAILURE\033[22;39m\n'
grep -A 2 '\*\*\*[1-9]' work.lst
exit 1
fi
tail -c +7 work.tmp.o > work.raw.o
cp work.raw.o work.o
dtran -d work.o > work.asm
rm -f work.tmp.o

./reconstruct-bin-header.py wrap --zones 16 --entries work.lst work.raw.o wlibo.bin
