#!/bin/sh
# Compile the paired array-index fixtures and require byte-identical object
# code.  The fixtures differ only by moving additive index constants into the
# corresponding declared lower bounds.
set -eu

mode=${1:-all}
case "$mode" in
    -host|-work|all) ;;
    *) echo "usage: $0 [-host|-work]" >&2; exit 2 ;;
esac

zero=tests/index_const_offset_zero.fixture
bound=tests/index_const_offset_bound.fixture
tmpbase=/tmp/index-offset-$$
deck=$tmpbase.dub
log=$tmpbase.lst

cleanup() {
    rm -f "$tmpbase".* ixsrc.utxt ixout.bin
}
trap cleanup EXIT HUP INT TERM

compare_objects() {
    label=$1
    first=$2
    second=$3
    if cmp -s "$first" "$second"; then
        echo "$label index-offset codegen equivalence: PASS"
        return
    fi
    echo "$label index-offset codegen equivalence: FAIL" >&2
    cmp "$first" "$second" >&2 || true
    return 1
}

compile_host() {
    src=$1
    out=$2
    ./base "$src" "$out" > "$log"
}

compile_work() {
    src=$1
    out=$2
    sed 's/{/<:/g;s/}/:>/g' < "$src" > ixsrc.utxt
    extent=$(./pashelp-source-extent.sh ixsrc.utxt)
    rm -f ixout.bin
    cat > "$deck" << EOF
*NAME ixoff
*disc:1/local
*file:pascom,42
*file:libc,43
*file:work,41
*file:ixsrc,44
*file:ixout,67,w
*system
*libra:42
*libra:41
*libra:43
*libra:22
*call pashelp
P 2 0 ${extent}B .
*call allmemory
*call *pascom
*copy:20,270000,670000
*table:exclude(pascontr)
*exclude
*to perso:670000
*end file
EOF
    timeout 3 dubna "$deck" > "$log"
    if [ ! -s ixout.bin ]; then
        cat "$log" >&2
        return 1
    fi
    ./reconstruct-bin-header.py extract ixout.bin "$out"
}

if [ "$mode" = -host ] || [ "$mode" = all ]; then
    compile_host "$zero" "$tmpbase.host-zero.o"
    compile_host "$bound" "$tmpbase.host-bound.o"
    compare_objects host "$tmpbase.host-zero.o" "$tmpbase.host-bound.o"
fi

if [ "$mode" = -work ] || [ "$mode" = all ]; then
    compile_work "$zero" "$tmpbase.work-zero.o"
    compile_work "$bound" "$tmpbase.work-bound.o"
    compare_objects work "$tmpbase.work-zero.o" "$tmpbase.work-bound.o"
fi
