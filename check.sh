#!/bin/sh
# Two-way self-host fixpoint: the library and the main module are compiled
# separately, so each is compared against its own emulator-compiled
# counterpart rather than a merged artifact.
# Usage: check.sh self-work.o work.o self-workmain.o workmain.o
ok=0
cmp "$1" "$2" || ok=1
cmp "$3" "$4" || ok=1
if [ $ok -eq 0 ]; then
    printf '\033[1;32mSUCCESS\033[22;39m\n'
else
    printf '\033[1;31mFAILURE\033[22;39m\n'
fi
exit $ok
