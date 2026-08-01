.PHONY: check test hotest worktest clean

# Self-host fixpoint (default target): the host-built work compiler
# (work.o/work.bin) recompiles work.p2c under the emulator (self.o); the two
# objects must be byte-identical.
check: self.o work.o
	./check.sh $^

# work.p2c compiled by the host-native compiler (base, from base.cc). This is
# the canonical build; the emulator base-module path is retired.
work.o work.bin: base work.p2c preprocess.py reconstruct-bin-header.py work.sh
	./work.sh

self.o: work.bin pascom.bin libc.bin work.p2c self.sh
	./self.sh
	grep -B 2 -A 1 'LINES STRUCTURE 1' self.lst

libc.bin: $(wildcard libc/*.madlen)
	./libc.sh

# Host-native compiler, the root of the bootstrap.
base: base.cc
	g++ -O3 -Wall -std=c++17 -o base base.cc

pascom.bin: build-pascom.dub
	dubna build-pascom.dub

# Tests compiled by the host compiler directly.
test hotest: base libc.bin
	./runhotests.sh

# Tests compiled by the emulator-hosted, self-hosted work compiler.
worktest: work.o libc.bin pascom.bin
	./runtests.sh -work

clean:
	rm -rf *.o tmp* *.lst *.asm *.bin *.utxt test_results test_results_hot
