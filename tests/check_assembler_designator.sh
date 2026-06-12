#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SRC="$ROOT/tests/fixtures/42_assembler_designator.src"
LISTING="${TMPDIR:-/tmp}/assembler_designator.lst"
TMP=$(mktemp "${TMPDIR:-/tmp}/assembler_designator.XXXXXX")

cleanup() {
    rm -f "$TMP"
}
trap cleanup EXIT

cd "$ROOT"
sed 's/{/<:/g;s/}/:>/g' < "$SRC" > tmpsrc.utxt
printf '                                                                                 \n' >> tmpsrc.utxt

cat > "$TMP" <<EOF
*NAME work
*disc:1/local
*file:pascom,42
*file:base,41
*file:libc,43
*file:tmpsrc,44
*     *pascom and pasmitxt
*libra:42
*     taking the base compiler module
*libra:41
*libra:22
*call pashelp
P 2 0 1000440000B .
*call *pascom
*copy:0,000000,000000
*libra:23
*call dtran(program)
*call setftn:one,long
*assem
*read:1
*end file
EOF

dubna "$TMP" | sed '1,/METAMORPH/d' > "$LISTING"

grep -Eq '(VJM|UJ)  *,ASMCALL' "$LISTING"
grep -q 'XTS' "$LISTING"
! grep -q 'P/PB' "$LISTING"
! grep -q 'P/BP' "$LISTING"
! grep -q 'KNTR' "$LISTING"
! grep -q '14, VTM ,74001B' "$LISTING"

echo "ASSEMBLER designator listing check passed: $LISTING"
