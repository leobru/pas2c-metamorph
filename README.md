# Step by step conversion of a BESM-6 Pascal compiler to a self-hosting BESM-6 C compiler

## To play

- Install https://github.com/besm6/dubna 
- Install https://github.com/besm6/pascal-re (only the `dtran` executable made from `dtran.cc` is of interest)
- `base.sh` compiles `base.pas`, the compiler written in BESM-6 Pascal without syntax modifications,
  which accepts the current language *in statu nascendi*, and generates a BESM-6 module of the result in `base.bin`;
- `work.sh` compiles `work.p2c`, the compiler written in the language *in statu nascendi*, which accepts its own syntax,
  using the binary in `base.bin` and generates a BESM-6 module of the result in `work.bin` (the module proper copied to `work.o`);
- `self.sh` compiles `work.p2c` using the binary in `work.bin` and generates a BESM-6 module of the result in `self.bin` (the module proper copied to `self.o`);
- `work.o` and `self.o` must match.
   
