#!/bin/sh
# Combine wlibo.bin (work.p2c's library, built by work.sh) and wmain.bin
# (workmain.p2c's MAIN, built by workmain.sh) into one file, work.bin, so
# every consumer script keeps attaching a single "work" module at unit 41
# -- exactly as before the split, unchanged for every script that was
# never touched for it.
#
# *call tcatalog needs a working compiler to build a catalog with (see
# work.sh's own comments); *perso:...,cont does not -- it merges the
# catalog directories two already-cataloged personal libraries carry,
# which is all wlibo.bin and wmain.bin already are on their own. Confirmed
# byte for byte: the combined file carries wmain's PASCOMPL/PROGRAM
# identity AND work's own NOPROGRA identity plus all its extern routine
# entries in one directory, and resolves correctly when attached at a
# single unit (verified against runsep.sh's own JCL with a single
# attachment, in place of its usual two).
set -e
for f in wlibo.bin wmain.bin; do
    if [ ! -f "$f" ]; then
        echo "$0: $f is missing" >&2
        exit 2
    fi
done
rm -f work.bin
cat << EOF > tmp$$
*NAME combine
*disc:1/local
*file:wlibo,44
*file:wmain,45
*file:work,67,w
*perso:44,cont
*perso:45,cont
*to perso:670000
*end file
EOF
( ulimit -t 20; exec dubna tmp$$ ) > combine.lst 2>&1
if ! grep -q 'ДЛИHA LIBRARY' combine.lst; then
    printf '\033[1;31mFAILURE\033[22;39m\n'
    tail -20 combine.lst
    rm -f tmp$$
    exit 1
fi
rm -f tmp$$
