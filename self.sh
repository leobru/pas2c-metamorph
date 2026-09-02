#!/bin/sh
# Self-host work.p2c under the DUBNA emulator: run the host-built work.bin
# compiler module against a preprocessed copy of its own source, producing
# self.bin/.o for comparison against the host-built work.o (check.sh).
#
rm -f wrksrc.bin
sed 's/{/<:/g;s/}/:>/g' < work.p2c | ./preprocess.py > wrksrc.utxt
src_extent=$(./pashelp-source-extent.sh wrksrc.utxt)
cat << EOF > tmp$$
*NAME work
*disc:1/local
*file:pascom,42
*file:libc,43
*file:work,41
*file:wrksrc,44
*file:self,67,w
*no list
*assem
 PASCONTR:,NAME,DTRAN  /01.06.84/
 PASINFOR:,LC,18
 P/SETEXF:,SUBP,
 RGEXPORT:,LC,1
 14,VTM,*0004B
 14,XTA,2
 ,UTC,RGEXPORT
 ,ATX,
 14,XTA,3
 ,UTC,PASINFOR
 ,ATX,3
 ,UJ,P/SETEXF
 *0004B:,TEXT,8HPASINPUT
 ,LOG,${src_extent} 400
 ,LOG,2
 ,INT,0
 ,END,
*system
*     *pascom and pasmitxt
*libra:42
*     taking the work compiler module
*libra:41
*libra:43
*libra:22
*call allmemory
*call *pascom
*copy:20,270000,670000
*table:exclude(pascontr)
*exclude
*to perso:670000
*end file
EOF
if [ "$1" = "-d" ]; then ln -f tmp$$ self.dub ; fi
rm -f self.o
length=`( ulimit -t 3; exec dubna tmp$$ ) | tee self.lst | grep 'HA LIBRARY' | cut -d ' ' -f 5`
length=$(($length-2))
grep -q 'LINES STRUCTURE 1' self.lst
if [ $? -ne 0 ]; then
echo '[1;31mFAILURE[22;39m'
exit 1
fi
echo Module length is $length zones
# Extract the exact-length object (matches the host-built work.o; a plain
# zone-granular dd would leave trailing padding and break the fixpoint cmp).
./reconstruct-bin-header.py extract self.bin self.o
dtran -d self.o > self.asm
rm -f tmp$$
