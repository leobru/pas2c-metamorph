check: self.o work.o
	./check.sh

self.o: work.o
	./self.sh

work.o: base.o work.p2c
	./work.sh

base.o: base.pas
	./base.sh

clean:
	rm -f *.o tmp*
