#!/bin/sh
# Compare base.pas vs work.p2c: sizeof in base only (ulimit -t 3).
set -e
ulimit -t 3
echo '=== runbase.sh (expect RESULT 1) ==='
./runbase.sh tests/31_base_work_sizeof.p2c 2>&1 | tee /tmp/r31b.out | tail -5
echo
echo '=== work compiler via pascom only (expect IDENTIFIER NOT DEFINED) ==='
(
  cat << 'EOF'
*NAME work
*disc:1/local
*file:pascom,42
*file:work,41
*file:libc,43
*file:tmpsrc,44
*libra:42
*libra:41
*libra:43
*libra:22
*call pashelp
P 2 0 1000440000B .
*call *pascom
EOF
  sed 's/{/<:/g;s/}/:>/g' < tests/31_base_work_sizeof.p2c
  echo '                                                                                 '
  cat << 'EOF'
*copy:0,000000,000000
*libra:43
*execute
*end file
EOF
) > /tmp/t31w.dub
dubna /tmp/t31w.dub 2>&1 | tee /tmp/r31w.out | grep -E 'RESULT|IDENTIFIER|ILLEGAL|ERRORS' || true
