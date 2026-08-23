# P2C compiler for BESM-6

This repository contains a self-hosting, C-shaped systems language compiler
for the BESM-6. The compiler emits Dubna-compatible BESM-6 modules and is
developed through a byte-identical bootstrap fixpoint.

## Requirements

- A C++17 compiler and GNU Make
- A POSIX shell, GNU core utilities, and Python 3
- [Dubna](https://github.com/besm6/dubna), including the `dubna` executable
- [`dtran`](https://github.com/besm6/pascal-re) from `pascal-re`

The `dubna` and `dtran` executables must be available through `PATH`.

## Compiler architecture

- `work.p2c` is the authoritative self-hosted compiler source.
- `base.cc` is its structurally aligned host-native C++ mirror and the root of
  the bootstrap.
- `work.o` is the compiler module produced by compiling `work.p2c` with the
  host compiler.
- `self.o` is produced when the work compiler recompiles `work.p2c` under
  Dubna.

Compiler changes are made in `work.p2c` and `base.cc` together. A successful
bootstrap requires `work.o` and `self.o` to be byte-identical.

## Build and test

Build the host-native compiler:

```sh
make base
```

Compile the self-hosted compiler source into a BESM-6 module:

```sh
make work.o
```

Verify the bootstrap fixpoint:

```sh
make check
```

Run the test suite through the host compiler and the work compiler:

```sh
make test
make worktest
```

`make worktest` is equivalent to `./runtests.sh -work`. The test runners
compile and execute the programs in `tests/` under Dubna and compare their
output with the corresponding `.expected` files.

## Language snapshot

P2C uses C-style declarations, expressions, statements, declarators, casts,
pointers, structures, unions, enumerations, and routines. A source module
contains declarations and uses a routine named `main` as its program entry
point.

Packed arrays specify the element width in bits:

```c
__packed int:8 bytes[12];
typedef __packed char alfa[6];
```

The language also supports C-style function pointers, nested routines,
declaration-site global initializers, `sizeof`, `offsetof`, sets, and external
`FORTRAN` and `ASSEMBLER` routines. The programs under `tests/` provide
executable examples of the supported syntax and runtime behavior.

## License

This project is available under the MIT License. See [LICENSE](LICENSE).
