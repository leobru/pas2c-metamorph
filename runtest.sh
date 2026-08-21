#!/bin/sh
rm -f tmpsrc.bin tmpsrc.txt
sed 's/{/<:/g;s/}/:>/g' < $1 > tmpsrc.utxt
src_extent=$(./pashelp-source-extent.sh tmpsrc.utxt)
cat << EOF > tmp$$
*NAME work
*disc:1/local
*file:pascom,42
*file:libc,43
*file:base,41
*file:tmpsrc,44
*     *pascom and pasmitxt
*libra:42
*     taking the base compiler module
*libra:41
*libra:22
*call pashelp
P 2 0 ${src_extent}B .
*call *pascom
*copy:0,000000,000000
*no load list
*perso:43,cont
*execute
*end file
EOF
if [ "$1" = "-d" ]; then ln -f tmp$$ run.dub ; fi
# Run through a file rather than a pipe: /bin/sh has no PIPESTATUS, so with
# `dubna | tee` the status inspected below is tee's and dubna's is lost --
# which is how a timed-out emulator used to look like a clean run.
timeout 3 dubna tmp$$ > runbase.lst
status=$?
cat runbase.lst
if [ $status -eq 124 ]; then
echo '[1;31mTIMEOUT[22;39m'
rm -f tmp$$
exit 124
fi
if [ $status -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
rm -f tmp$$
