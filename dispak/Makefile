.PHONY: check test hotest worktest clean ccom.bin libc.bin

# Paths are relative to the repo root; run as: make -C dispak
# or: make -f dispak/Makefile from the root.

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
VPATH := $(ROOT)
export PATH := $(ROOT):$(PATH)

check: self.o work.o
	cd $(ROOT) && ./check.sh $^

self.o: $(ROOT)/work.bin $(ROOT)/ccom.bin $(ROOT)/libc.bin $(ROOT)/work.p2c
	cd $(ROOT) && ./dispak/self.sh
	grep -B 2 -A 1 'LINES STRUCTURE 1' $(ROOT)/self.lst

work.o work.bin: $(ROOT)/base $(ROOT)/work.p2c
	cd $(ROOT) && ./work.sh

base: $(ROOT)/base.cc
	cd $(ROOT) && g++ -O3 -Wall -std=c++17 -o base base.cc

libc.bin: $(wildcard $(ROOT)/libc/*.madlen)
	cd $(ROOT) && ./dispak/libc.sh

ccom.bin: $(ROOT)/build-ccom.dub
	cd $(ROOT) && ./dispak/ccom.sh

test hotest: $(ROOT)/base $(ROOT)/libc.bin
	cd $(ROOT) && ./dispak/runhotests.sh

worktest: $(ROOT)/work.o $(ROOT)/libc.bin $(ROOT)/ccom.bin
	cd $(ROOT) && ./dispak/runtests.sh -work

clean:
	cd $(ROOT) && rm -rf *.o tmp* *.lst *.asm *.bin *.utxt \
	    test_results test_results_hot test_results_dispak test_results_hot_dispak \
	    30[0-9][0-9] 3[1-9][0-9][0-9]
