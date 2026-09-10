#!/bin/sh
# Self-host the split work compiler under the DUBNA emulator: run the
# combined work.bin compiler (workmain.p2c's MAIN plus work.p2c's library,
# merged into one file by combine.sh) against fresh preprocessed copies of
# each own source, producing self-work.o and self-wmain.o for comparison
# against the host-built work.o and wmain.o (check.sh) - one cmp per
# module, not a merged artifact.
#
# DUBNA file names are six characters, which is why every *file: card
# below uses a short scratch name rather than the module's own name.
compile_one () {   # src.p2c dubna_short_name friendly_prefix
    src=$1
    short=$2
    dest=$3
    rm -f "$short.bin"
    sed 's/{/<:/g;s/}/:>/g' < "$src" | ./preprocess.py > "$short.utxt"
    src_extent=$(./pashelp-source-extent.sh "$short.utxt")
    cat << EOF > tmp$$
*NAME work
*disc:1/local
*file:ccom,42
*file:libc,43
*file:work,41
*file:$short,44
*file:o$short,67,w
*no list
*assem
 PASCONTR:,NAME,DTRAN  /01.06.84/
 PASINFOR:,LC,18
 C/SETEXF:,SUBP,
 RGEXPORT:,LC,1
 14,VTM,*0004B
 14,XTA,2
 ,UTC,RGEXPORT
 ,ATX,
 14,XTA,3
 ,UTC,PASINFOR
 ,ATX,3
 ,UJ,C/SETEXF
 *0004B:,TEXT,8HPASINPUT
 ,LOG,${src_extent} 400
 ,LOG,2
 ,INT,0
 ,END,
*system
*     ccom and pasmitxt
*libra:42
*     taking the work compiler module
*libra:41
*libra:43
*libra:22
*call allmemory
*call ccom
*copy:20,270000,670000
*table:exclude(pascontr)
*exclude
*to perso:670000
*end file
EOF
    rm -f "o$short.bin"
    length=`( ulimit -t 8; exec dubna tmp$$ ) | tee "self-$dest.lst" | grep 'HA LIBRARY' | cut -d ' ' -f 5`
    length=$(($length-2))
    grep -q 'LINES STRUCTURE 1' "self-$dest.lst"
    if [ $? -ne 0 ]; then
        echo "$0: self-compile of $src failed"
        printf '\033[1;31mFAILURE\033[22;39m\n'
        exit 1
    fi
    echo "$src module length is $length zones"
    ./reconstruct-bin-header.py extract "o$short.bin" "self-$dest.o"
    dtran -d "self-$dest.o" > "self-$dest.asm"
    rm -f tmp$$ "$short.utxt" "o$short.bin"
}

compile_one work.p2c wlsrc work
compile_one workmain.p2c wmsrc wmain
