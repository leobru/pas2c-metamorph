`l3bool13z` is a local boolean inside `formOperator`. It is a codegen context flag: **“am I allowed to turn a too-complex packed lvalue into a loaded rvalue?”**

It is initialized to `true` at the start of `formOperator`.

It is temporarily set to `false` in two important places:

```pascal
if (curOP = ASSIGNOP) then
    l3bool13z := false;
genFullExpr(exprToGen@.expr1);
if (curOP = ASSIGNOP) then
    l3bool13z := true;
```

and in `genRMWAssign` while materializing the lhs address:

```pascal
l3bool13z := false;
genFullExpr(lhsExpr);
l3bool13z := true;
```

The only meaningful read is in `GETFIELD` codegen, when the base expression is already `stPACKED`:

```pascal
stPACKED:
    if (not l3bool13z) then
        error(errUsingVarAfterIndexingPackedArray)
    else {
        startIL1;
        insnList@.shift := curIdRec@.shift;
    }
```

So the intent is:

- In normal rvalue expression context, `l3bool13z = true`.
  If code sees a packed-field selection after a dynamically indexed packed array (`stPACKED`), it may recover by loading the dynamic packed element and continuing from that value.

- In lvalue/address context, `l3bool13z = false`.
  That recovery is forbidden, because loading the element would destroy the ability to store back to the original location. So the compiler reports `errUsingVarAfterIndexingPackedArray`.

In plainer terms: `l3bool13z` is a “rvalue fallback allowed” flag. The name is just compiler-translation residue; a better name would be something like:

```pascal
allowPackedRvalueFallback
```

or, more specifically:

```pascal
allowSt2FieldLoad
```

It is especially relevant to `RMWASSIGN`: the implementation sets it false while computing the lhs because `+=`, `++`, etc. need a real store-capable lvalue, not a loaded copy.
