# P2C Compiler Test Suite

This directory contains test programs for the p2c compiler (Pascal-to-C hybrid language).

## Test Organization

Each test consists of:
- **`.p2c` file**: The test program source code
- **`.expected` file** (optional): Expected output after execution
- **`.should_fail` file** (optional): Marker that compilation/execution is expected to fail

## Running Tests

### Run all tests
```bash
./runtests.sh
```

### Run specific test(s)
```bash
./runtests.sh 01_hello
./runtests.sh 0[1-5]_*
```

### Run single test manually
```bash
./runbase.sh tests/01_hello.p2c
```

## Test Categories

### Basic Features (01-10)
- 01: Hello world
- 02: C-style braces instead of begin/end
- 03: Integer arithmetic
- 04-07: Bitwise operations (&, |, ^, ~)
- 08: If statement
- 09: While loop
- 10: For loop

### Procedures and Functions (11-13)
- 11: Procedure (void function)
- 12: Function with return value
- 13: Recursive function (factorial)

### Data Structures (14-20)
- 14: Arrays
- 15: Boolean logic
- 16: Comparison operators
- 17: Nested blocks
- 18: Switch/case statement
- 19: String comparison
- 20: Pointer basics

### Advanced Features (21-25)
- 21-22: Shift operators (<< >>)
- 23: Struct/record types
- 24: Enum types
- 25: Character operations

## Pascal vs P2C Differences

### Syntax Differences

| Feature | Pascal | P2C |
|---------|--------|-----|
| Blocks | `begin ... end` | `{ ... }` |
| Bitwise AND | N/A | `&` |
| Bitwise OR | N/A | `\|` |
| Bitwise XOR | N/A | `^` |
| Bitwise NOT | N/A | `~` |
| Left shift | N/A | `<<` |
| Right shift | N/A | `>>` |
| Procedure | `procedure` | `void` |
| Record | `record ... end` | `struct { ... }` |
| Case | `case ... of ... end` | `switch (...) of ... end` |
| Enumeration | `(RED, GREEN, BLUE)` | `enum { RED, GREEN, BLUE }` |

### Semantic Differences

1. **Expression statements**: P2C allows function calls as standalone statements
2. **Return statement**: P2C supports explicit `return` (if implemented)
3. **Pointer arithmetic**: P2C supports pointer arithmetic operations
4. **C-style operators**: P2C uses C-style bitwise and shift operators

## Test Output Format

When a test runs successfully:
```
Running test: 01_hello ... PASS
```

When a test fails:
```
Running test: 03_arithmetic ... FAIL (output mismatch)
  Expected:
  10 + 3 = 13
  Got:
  10 + 3 = 12
```

## Adding New Tests

1. Create `tests/NN_testname.p2c` with your test program
2. Create `tests/NN_testname.expected` with expected output
3. If test should fail to compile, create `tests/NN_testname.should_fail`
4. Run `./runtests.sh NN_testname` to verify

## Test Program Structure

All test programs should follow this structure:

```pascal
(*=p-,t-,s8,u-,y+,k9,l0*)
program testname(output);
{ (* declarations *)
    (* code *)
}
.
```

The header comment `(*=p-,t-,s8,u-,y+,k9,l0*)` is required for BESM-6 compilation.

## Notes

- Tests are executed on the BESM-6 emulator via `dubna`
- Compilation uses the base compiler from `base.bin`
- Output is captured after the `*EXECUTE` line in the emulator output
- Tests have a 10-second timeout
- String literals in BESM-6 Pascal are fixed-width packed arrays
