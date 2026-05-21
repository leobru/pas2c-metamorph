check: self.o work.o
	./check.sh $^

checks9: selfs9.o works9.o
	./check.sh $^

self.o: work.o pascom.bin libc.bin
	./self.sh

work.o: base.o work.p2c pascom.bin
	./work.sh

selfs9.o: work.o pascom.bin libc.bin
	./selfs9.sh

works9.o: base.o work.p2c pascom.bin
	./works9.sh

libc.bin: libc.src
	./libc.sh

base.o: base.pas
	./base.sh

pascom.bin: build-pascom.dub
	dubna build-pascom.dub

test: base.o libc.bin pascom.bin
	./runtests.sh

clean:
	rm -rf *.o tmp* *.lst *.asm *.bin *.utxt test_results
