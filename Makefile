check: self.o work.o
	./check.sh $^

self.o: work.o pascom.bin libc.bin
	./self.sh

work.o: base.o work.p2c pascom.bin
	./work.sh

libc.bin: $(wildcard libc/*.madlen)
	./libc.sh

base.o: base.pas
	./base.sh

# Host-native base compiler (port in progress, see port/NOTES.md)
base: base.cc
	g++ -O3 -Wall -std=c++17 -o base base.cc

pascom.bin: build-pascom.dub
	dubna build-pascom.dub

test: base.o libc.bin pascom.bin
	./runtests.sh

worktest: work.o libc.bin pascom.bin
	./runtests.sh -work

clean:
	rm -rf *.o tmp* *.lst *.asm *.bin *.utxt test_results
