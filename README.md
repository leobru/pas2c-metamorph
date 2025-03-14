# Step by step conversion of a BESM-6 Pascal compiler to a self-hosting BESM-6 C compiler

## To play

- Install https://github.com/besm6/dispak 
- Install https://github.com/besm6/pascal-re (only the `dtran` executable made from `dtran.cc` is of interest)
- `mkdir ~/.besm6; touch ~/.besm6/1234`
- `base.sh` compiles `base.pas`, the compiler written in BESM-6 Pascal without syntax modifications,
  which accepts the current language *in statu nascendi*, and generates a BESM-6 module of the result starting from zone 0 of the volume 1234;
- `work.sh` compiles `work.p2c`, the compiler written in the language *in statu nascendi*, which accepts its own syntax,
  using the binary at vol 1234 zone 0, and generates a BESM-6 module of the result starting from zone 0100 of the volume 1234 (copied to `work.o`);
- `self.sh` compiles `work.p2c` using the binary at vol 1234 zone 0100,
  and generates a BESM-6 module of the result starting from zone 0200 of the volume 1234 (copied to `self.o`);
- `work.o` and `self.o` must match.
   
