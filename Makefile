check: self.o work.o
	./check.sh

self.o: work.o pascom.bin
	./self.sh

work.o: base.o work.p2c pascom.bin
	./work.sh

base.o: base.pas
	./base.sh

clean:
	rm -f *.o tmp* *.lst *.asm
