.PHONY: check test hotest worktest basectest workctest clean

selfc: workc.bin pascom.bin libc.bin work-c.p2c self-c.sh
	./self-c.sh
	grep -B 2 -A 1 'LINES STRUCTURE 1' selfc.lst

# Self-host fixpoint: the host-built work compiler (work.o/work.bin)
# recompiles work.p2c under the emulator (self.o); the two objects must be
# byte-identical.
check: self.o work.o
	./check.sh $^

# work.p2c compiled by the host-native compiler (base.cc). This is the
# canonical build; the emulator base-module path (base.pas) is retired.
work.o work.bin: base work.p2c preprocess.py reconstruct-bin-header.py work.sh
	./work.sh

self.o: work.bin pascom.bin libc.bin work.p2c self.sh
	./self.sh

libc.bin: $(wildcard libc/*.madlen)
	./libc.sh

# Host-native compiler, the root of the bootstrap.
base: base.cc
	g++ -O3 -Wall -std=c++17 -o base base.cc

# Experimental C-declarator-only fork of base.cc (see the-idea-is-to-
# kind-naur plan). Not part of the work.p2c bootstrap; validated against
# tests-c/ before its design is ported back into base.cc/work.p2c.
base-c: base-c.cc
	g++ -O3 -Wall -std=c++17 -o base-c base-c.cc

pascom.bin: build-pascom.dub
	dubna build-pascom.dub

# work-c.p2c compiled by the host-native base-c compiler. Mirrors work.o/
# work.bin's own rule above, for the experimental C-declarator fork.
workc.o workc.bin: base-c work-c.p2c preprocess.py reconstruct-bin-header.py work-c.sh
	./work-c.sh

# Tests compiled by the host compiler directly.
test hotest: base libc.bin
	./runhotests.sh

# tests-c/ compiled by base-c directly.
basectest: base-c
	./run-base-c-tests.sh

# Tests compiled by the emulator-hosted work compiler.
worktest: work.o libc.bin pascom.bin
	./runtests.sh -work

# tests-c/ compiled by the emulator-hosted, self-hosted work-c compiler.
workctest: workc.bin libc.bin pascom.bin
	./run-workc-tests.sh

clean:
	rm -rf *.o tmp* *.lst *.asm *.bin *.utxt test_results test_results_hot \
		test_results_basec test_results_workc
