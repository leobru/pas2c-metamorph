.PHONY: check test hotest worktest clean

# Self-host fixpoint (default target): the split compiler (workmain.p2c has
# MAIN, built as wmain.o/wmain.bin -- "workmain" is 8 characters and every
# DUBNA *file: name is at most 6; work.p2c is everything else, an
# extern-exported library) recompiles its own two sources under the
# emulator; each module's emulator-compiled object must be byte-identical
# to its own host-compiled one. Two separate cmps, not a merged artifact --
# see separate_main_plan.md.
check: self-work.o work.o self-wmain.o wmain.o
	./check.sh $^

# workmain.p2c compiled by the host-native compiler (base, from base.cc).
# Fast path, no dubna: it only ever needs to be jumped into by its own start
# word, never resolved by name from another module.
wmain.o wmain.bin: base workmain.p2c preprocess.py reconstruct-bin-header.py workmain.sh
	./workmain.sh

# work.p2c: work.o is the host-native compile (base), the make-check
# reference; wlibo.bin is the name-resolvable library staged for
# combine.sh, built by the same host compile plus
# reconstruct-bin-header.py's --entries (a real catalog directory built
# from base.cc's own entry-point dump, no DUBNA compilation and no prior
# wlibo.bin/wmain.bin needed -- see work.sh's own comments and
# separate_main_plan.md's "Bootstrap regression"). wlibo.bin is not
# attached by any consumer script directly -- combine.sh merges it with
# wmain.bin into the deployable work.bin every script attaches, unchanged,
# at unit 41, exactly as before the split.
work.o wlibo.bin: base work.p2c preprocess.py reconstruct-bin-header.py work.sh
	./work.sh

work.bin: wlibo.bin wmain.bin combine.sh
	./combine.sh

self-work.o self-wmain.o: work.bin ccom.bin libc.bin work.p2c workmain.p2c self.sh
	./self.sh
	grep -B 2 -A 1 'LINES STRUCTURE 1' self-work.lst
	grep -B 2 -A 1 'LINES STRUCTURE 1' self-wmain.lst

libc.bin: $(wildcard libc/*.madlen)
	./libc.sh

# Host-native compiler, the root of the bootstrap.
base: base.cc
	g++ -O3 -Wall -std=c++17 -o base base.cc

# Host-native b6as/Unix-style assembly dump; not in the bootstrap.
baseasm: baseasm.cc
	g++ -O3 -Wall -std=c++17 -o baseasm baseasm.cc

ccom.bin: build-ccom.dub
	dubna build-ccom.dub

# Tests compiled by the host compiler directly.
test hotest: base libc.bin
	./runhotests.sh

# Tests compiled by the emulator-hosted, self-hosted work compiler.
worktest: work.bin libc.bin ccom.bin
	./runtests.sh -work

clean:
	rm -rf *.o tmp* *.lst *.asm *.bin *.utxt test_results test_results_hot
