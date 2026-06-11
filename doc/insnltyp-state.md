`insnltyp.st` is the addressing sub-state for an `ilLVAL` instruction list. It tells codegen whether the current lvalue/rvalue denotes a normal word, a statically known packed subfield, or a dynamically indexed packed element.

Declared here: [base.pas](//wsl$/Ubuntu/home/leob/git/leobru/pas2c-metamorph/base.pas:228)

```pascal
state = (stWORD, stSLICE, stPACKED);
...
insnltyp = record
    ...
    st: state;
    width, shift: integer
end;
```

Meaning:

- `stWORD`: normal addressable full-word object.
  `disp`, `payload.i`, and `addrmd` are enough to form the address. Loads/stores become ordinary `XTA`/`ATX` style operations.

- `stSLICE`: statically known packed field or packed array element inside a word.
  `disp` points to the containing word, while `shift` and `width` describe the bit slice. `prepLoad` extracts the slice with shifts/masks; `prepStore` masks and merges the slice back into the containing word.

- `stPACKED`: dynamically indexed packed array element.
  The bit position cannot be represented as a fixed `disp + shift`. Codegen emits runtime helper-based access, using helpers like `P/LDAR` / `P/RR` for load and `P/STAR` for store. Further packed indexing/fielding after this is restricted and may raise `errUsingVarAfterIndexingPackedArray`.

Main usage:

- New ordinary variables start as `stWORD` in `GETVAR` generation.
- Packed fields set `st := stSLICE` and fill `shift`/`width`.
- Constant indexing into a packed array also sets `stSLICE`, folding the index into `disp` plus a fixed bit `shift`.
- Nonconstant indexing into a packed array sets `stPACKED`, because the packed element location must be computed at runtime.
- `prepLoad` and `prepStore` branch heavily on `st` to decide whether to emit simple word access, bit extraction/merge, or helper calls.
- Some contexts require simple addresses, so they reject non-`stWORD`; for example `SETREG9` errors if `insnList@.st <> stWORD`.

So, in one sentence: `st` is the “packed-address shape” of the current intermediate lvalue, with `stWORD` = whole word, `stSLICE` = fixed bit slice, and `stPACKED` = runtime-computed packed element.
