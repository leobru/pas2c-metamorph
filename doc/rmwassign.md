`RMWASSIGN` is clever, but only correct for a narrower set of lvalues than it currently accepts.

The implementation at [base.pas](//wsl$/Ubuntu/home/leob/git/leobru/pas2c-metamorph/base.pas:3593) does this:

1. For “complex” lvalues, generate the lhs once.
2. Convert its address to `M14` via `setAddrTo(14)`.
3. Push that address twice.
4. Replace the lhs with synthetic `STKLVAL`.
5. Generate `lhs = lhs op rhs`, where each `STKLVAL` pops one saved address.

That works for simple full-word lvalues. The problem is that an lvalue is not always just an address.

**Main Correctness Problems**

1. **Packed lvalues lose `st/width/shift`.**

For packed fields/elements, the lvalue descriptor is:

```pascal
addr + stSLICE + width + shift
```

or sometimes `stPACKED` for runtime packed indexing. `setAddrTo(14)` preserves only the containing word address. Then `STKLVAL` is rebuilt as:

```pascal
st := stWORD;
addrmd := 16;
payload.i := 0;
disp := 0;
```

So `x.packed_field += 1` can become “load/store the whole containing word” instead of “load/store this bit slice”. That is the biggest bug.

2. **The shortcut evaluates some lvalues twice.**

This predicate is too optimistic:

```pascal
needsMater := (lhsExpr@.op <> GETVAR) and
              ((lhsExpr@.op <> GETFIELD) or
               (lhsExpr@.expr1@.op <> GETVAR)) and
              ((lhsExpr@.op <> GETELT) or
               (lhsExpr@.expr2@.op <> GETVAR));
```

`GETELT` with a variable index is treated as safe to re-walk. But:

```c
a[i] += (i = 2)
```

will evaluate `i` after RHS mutation in the synthetic `lhs = lhs + rhs` tree. Even if your language does not promise C evaluation order, the whole point of `RMWASSIGN` is “evaluate lhs once”; this shortcut violates that.

It is worse for something like:

```c
p[i] += f()
```

where the base expression may also be nontrivial, but the top-level `GETELT` shortcut only checks that the index is `GETVAR`.

3. **Packed store and stack-address `STKLVAL` do not compose cleanly.**

One tempting fix is to make `STKLVAL` carry `stSLICE/width/shift`. But `prepStore` for `stSLICE` prepends a `PUSH` to save the value while it reads/masks/merges the containing word. Current `STKLVAL` uses `KWTC+SP` to pop the saved address. Those two uses of `SP` collide: the packed-store value push can put the value above the saved address.

So preserving `stSLICE` in the current stack-based `STKLVAL` is not enough.

**Practical Fixes**

The safest incremental fix:

```pascal
if needsMater then {
    l3bool13z := false;
    genFullExpr(lhsExpr);
    l3bool13z := true;

    if insnList@.st <> stWORD then {
        error(errVarTooComplex);
        exit;
    };

    setAddrTo(14);
    ...
}
```

Then tighten `needsMater`:

- Only skip materialization for truly stable lvalues.
- `GETVAR` is safe.
- `GETFIELD` is safe only if its base is stable and address-only.
- `GETELT` with variable index should materialize, not shortcut.
- Packed lvalues with only constant/static addressing can re-walk, because the address does not depend on RHS mutation and packed metadata is preserved by normal codegen.

Better long-term fix:

Create a synthetic lvalue that stores the computed address in a compiler temp local, not on the BESM stack.

Shape:

1. Generate lhs once.
2. If `stWORD`, existing stack method is okay.
3. If `stSLICE`, save containing-word address into a temp local.
4. Synthetic lvalue reloads address using `WTC temp`, not `KWTC SP`.
5. Synthetic lvalue also carries saved `st`, `width`, `shift`.

That avoids the `SP` collision in packed `prepStore`.

For `stPACKED`, either:

- initially reject `RMWASSIGN` as too complex, or
- save the full dynamic packed-access descriptor required by `P/LDAR`/`P/STAR`, which is a bigger change.

My recommended path: first add the guard against `st <> stWORD` in the materialized path and make `needsMater` conservative. That prevents silent wrong-code. Then add temp-address synthetic lvalues for `stSLICE` packed fields/elements once the simple case is stable.
