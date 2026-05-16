check: self.o work.o
	./check.sh

self.o: work.o pascom.bin libc.bin
	./self.sh

work.o: base.o work.p2c pascom.bin
	./work.sh

libc.bin: libc.src
	./libc.sh

base.o: base.pas
	./base.sh

pascom.bin: build-pascom.dub
	dubna build-pascom.dub

clean:
	rm -f *.o tmp* *.lst *.asm *.bin *.utxt
