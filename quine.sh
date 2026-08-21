#!/bin/sh
# quine.p2c prints its own source.  Compile it, run it, and compare what it
# printed with the file it was compiled from.
#
#   ./quine.sh          the host compiler (./base), run under DUBNA
#   ./quine.sh -work    the self-hosted compiler (work.bin), likewise
#
# The emulator writes the apostrophe as U+2032 PRIME in its output charset, so
# that one character is mapped back before the comparison, and the padding it
# puts at the end of every line is stripped from both sides.  Nothing else is
# touched: the counts printed on a pass are of the text as it stands.
src=quine.p2c
out=quine.out
norm=quine.norm
plain=quine.src

case "$1" in
    -work) runner=./runworktest.sh ;;
    "")    runner=./runhotest.sh ;;
    *)     echo "usage: $0 [-work]" >&2; exit 2 ;;
esac
if [ ! -f "$src" ]; then echo "$0: $src is missing" >&2; exit 2; fi

$runner "$src" > $out 2>&1
# the program's own output: between the *EXECUTE card and the timing rule
sed -n '/\*EXECUTE/,/^----/p' $out | sed '1d;$d' | sed "s/′/'/g;s/[[:space:]]*\$//" > $norm
sed 's/[[:space:]]*$//' $src > $plain

if [ ! -s $norm ]; then
    printf 'quine: \033[1;31mFAIL\033[22;39m (it printed nothing)\n'
    grep -E '^Error|OШИБ|TIMEOUT' $out | head -3
    rm -f $norm $plain
    exit 1
fi

if cmp -s $norm $plain; then
    printf 'quine: \033[1;32mPASS\033[22;39m  %s lines, %s bytes reproduced\n' \
        "$(wc -l < $norm)" "$(wc -c < $norm)"
    rm -f $norm $plain $out
    exit 0
fi
printf 'quine: \033[1;31mFAIL\033[22;39m\n'
diff $norm $plain | head -20
echo "  (its output is in $out)"
rm -f $norm $plain
exit 1
