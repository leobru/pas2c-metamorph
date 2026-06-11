`addrmd` is the address-mode tag inside `insnltyp`. It tells codegen how to interpret the address fields of an `ilLVAL` operand.

For an `ilLVAL` value, these fields work together:

```pascal
addrmd      (* how the address is represented *)
payload.i   (* base instruction/template or base offset *)
disp        (* displacement *)
st          (* full word vs packed bit-field state *)
```

The rough meaning of `addrmd` is:

- `0..14`: address is relative to BESM index register `addrmd`.
  Codegen emits something like `indexreg[addrmd] + UTC`, then uses `payload.i + XTA/ATX` with `disp`.

- `15`: address is currently in ACC.
  Codegen emits the `mcACC2ADDR` macro, effectively converting ACC into an addressable form before the load/store.

- `16`: address has already been arranged by prior code, usually through `WTC`/computed addressing.
  The following load/store can use offset `0`, for example after dereference or synthetic stack lvalue handling.

- `17`: address is preceded by an emitted `UTC` literal/global reference.
  This is used for some global/symbol-table-addressed objects; `setAddrTo` knows how to rewrite or combine that pending `UTC`.

- `18`: direct/base-template mode.
  This is the common initial mode for variables. `payload.i` already contains the base instruction/register template, and `disp` is the offset.

You can see the main interpretation in `prepLoad` and `prepStore`: they first look at `addrmd` to prepare the address, then use `st` to decide whether the actual access is a simple word access, packed-bit extraction/merge, or helper-based packed access.

A plain local variable starts approximately as:

```pascal
ilm := ilLVAL;
addrmd := 18;
payload.i := curIdRec@.offset;  (* base/frame template *)
disp := curIdRec@.high.i;       (* variable offset *)
st := stWORD;
```

After dereference, `genDeref` changes the representation so the loaded pointer becomes the new address:

```pascal
addrmd := 16;
disp := 0;
payload.i := 0;
```

And `setAddrTo(reg)` is the normalizer: given whatever `addrmd` representation the operand currently has, it emits instructions to materialize the address into an index register, then rewrites the `insnList` as:

```pascal
ilm := ilLVAL;
addrmd := reg;
disp := 0;
payload.i := 0;
```

So, short version: `addrmd` is not the object’s type or packed-state. It is the low-level addressing form selector for `payload.i + disp`, telling the emitter whether the address is direct, index-register-relative, in ACC, behind a `UTC/WTC`, or already computed.
