#!/bin/sh
# Check the ordered normalization-mode instructions in one compiled test.
# A test's .ntr sidecar holds one NTR operand per line; bare `,NTR,` is 0.
# Addresses and all other instructions are deliberately ignored.
set -eu

if [ $# -lt 2 ] || [ $# -gt 3 ]; then
    echo "usage: $0 host|work test.p2c [host-object]" >&2
    exit 2
fi

compiler=$1
src=$2
expected=${src%.p2c}.ntr
root=$(CDPATH= cd -- "$(dirname "$0")" && pwd)

if [ ! -f "$src" ] || [ ! -f "$expected" ]; then
    echo "$0: missing source or NTR sidecar for $src" >&2
    exit 2
fi

case $compiler in
host|work) ;;
*)
    echo "$0: compiler must be host or work" >&2
    exit 2
    ;;
esac

tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/p2c-ntr.XXXXXX")
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
object=$tmpdir/test.o

if [ "$compiler" = host ] && [ $# -eq 3 ]; then
    object=$3
elif [ "$compiler" = host ]; then
    sed 's/{/<:/g;s/}/:>/g' < "$src" > "$tmpdir/ntrsrc.utxt"
    if ! "$root/base" "$tmpdir/ntrsrc.utxt" "$object" > "$tmpdir/compile.lst"; then
        cat "$tmpdir/compile.lst" >&2
        exit 1
    fi
else
    for module in ccom libc work; do
        if [ ! -f "$root/$module.bin" ]; then
            echo "$0: $module.bin is required for a work check" >&2
            exit 2
        fi
        ln -s "$root/$module.bin" "$tmpdir/$module.bin"
    done
    sed 's/{/<:/g;s/}/:>/g' < "$src" > "$tmpdir/ntrsrc.utxt"
    extent=$("$root/pashelp-source-extent.sh" "$tmpdir/ntrsrc.utxt")
    cat > "$tmpdir/check.dub" <<EOF
*NAME ntrcheck
*disc:1/local
*file:ccom,42
*file:libc,43
*file:work,41
*file:ntrsrc,44
*file:ntrout,67,w
*libra:42
*libra:43
*libra:41
*libra:22
*call pashelp
P 2 0 ${extent}B .
*call ccom
*copy:20,270000,670000
*table:exclude(pascontr)
*exclude
*to perso:670000
*end file
EOF
    if ! (cd "$tmpdir" && ulimit -t 10 && exec dubna check.dub > compile.lst 2>&1); then
        cat "$tmpdir/compile.lst" >&2
        exit 1
    fi
    if ! grep -q 'MAXHEAP' "$tmpdir/compile.lst" ||
       [ ! -f "$tmpdir/ntrout.bin" ]; then
        cat "$tmpdir/compile.lst" >&2
        exit 1
    fi
    "$root/reconstruct-bin-header.py" extract "$tmpdir/ntrout.bin" "$object"
fi

dtran -d "$object" > "$tmpdir/test.asm" 2> "$tmpdir/dtran.err" || {
    cat "$tmpdir/dtran.err" >&2
    exit 1
}

awk '
/,NTR,/ {
    line = $0
    sub(/^.*NTR,/, "", line)
    sub(/[[:space:]].*$/, "", line)
    if (line == "")
        line = "0"
    print line
}
' "$tmpdir/test.asm" > "$tmpdir/actual.ntr"

if ! diff -u "$expected" "$tmpdir/actual.ntr"; then
    echo "$src: unexpected NTR sequence from $compiler compiler" >&2
    exit 1
fi
