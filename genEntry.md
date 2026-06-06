# `genEntry` in `base.pas`

`procedure genEntry` generates the BESM-6 code sequence for **calling a
procedure or function**. It is invoked from the code-generation dispatcher when
the current expression's opcode is `ALNUM` (the AST opcode for "actual
subroutine call").

## 1. Input data

```pascal
procedure genEntry;
var
    l5exp1z, l5exp2z: eptr;
    l5idr3z, l5idr4z, l5idr5z, l5idr6z: irptr;
    l5bool7z, l5bool8z, l5bool9z, l5bool10z, l5bool11z: boolean;
    l5var12z, l5var13z, l5var14z: word;
    l5var15z: integer;
    l5var16z, l5var17z, l5var18z, l5var19z: word;
    l5inl20z: @insnltyp;
    l5op21z: operator; paramClass: idclass;
```

`genEntry` reads the global `exprToGen` (`@eptr`):

- `exprToGen^.id2` — `irptr` of the routine being called (`l5idr5z`).
- `exprToGen^.expr1` — head of the **argument chain** (`l5exp1z`). Each node in
  the chain has `expr1 = next-arg` and `expr2 = this-arg`.

The argument chain is consumed **back-to-front** (last argument first) by
walking `expr1` repeatedly.

## 2. Setup phase

```pascal
{ (* genEntry *)
    l5exp1z := exprToGen@.expr1;
    l5idr5z := exprToGen@.id2;
    l5bool7z := (l5idr5z@.typ = NIL);
    l5bool9z := (l5idr5z@.list = NIL);
    if (l5bool7z) then
        l5var13z.i := 3 else l5var13z.i := 4;
    l5var12z.m := l5idr5z@.flags;
    l5bool10z := (21 in l5var12z.m);
    l5bool11z := (24 in l5var12z.m);
    if (l5bool9z) then {
        l5var14z.i := argCount(l5idr5z);
        l5idr6z := l5idr5z@.argList;
    } else {
        l5var13z.i := l5var13z.i + 2;
    };
```

Booleans set the call style:

| Var | Meaning |
|---|---|
| `l5bool7z` | `typ = NIL` → **procedure** (no result), else **function**. Affects the result-slot accounting (3 vs 4 stack words). |
| `l5bool9z` | `list = NIL` → **direct call** (full routine info available, so arguments are pushed positionally and `argList` walks the formals). `list ≠ NIL` → **indirect/formal call** (calling through a routine parameter; arg layout is generic). |
| `l5bool10z` | bit 21 — **Fortran** call (different prologue/epilogue, P/MF, KNTR+2, KNTR+7 / P/FM). |
| `l5bool11z` | bit 24 — pass everything as **formal/by-reference**, regardless of actual class. |
| `l5var12z.m` | snapshot of routine flags. |
| `l5var13z.i` | stack-frame slot count (3 or 4); +2 more for indirect calls (room for the closure/code+frame pair). |
| `l5var14z.i` | for direct calls, the formal-argument count. |
| `l5idr6z` | current formal pointer (for direct calls), advances as arguments are consumed. |

Then the new instruction-list buffer is allocated:

```pascal
    new(insnList);
    insnList@.head := NIL;
    insnList@.next := NIL;
    insnList@.typ := l5idr5z@.typ;
    insnList@.regsused := (l5idr5z@.flags + [7:15]) * [0:8, 10:15];
    insnList@.ilm := ilVALINACC;
```

`regsused` starts with **bits 7..15 except bit 9** added to the routine's
clobber set, so the register allocator knows the callee will trash the display
registers and accumulator-related bits.

`ilm = ilVALINACC` declares the resulting "instruction list" produces an **rvalue in
the accumulator**.

Frame reservation:

```pascal
    if (l5bool10z) then {
        l5bool8z := not l5bool7z;
        if (checkFortran) then {
            addToInsnList(getHelperProc(92)); (* "P/MF" *)
        }
    } else {
        l5bool8z := true;
        if (not l5bool9z) and (l5exp1z <> NIL)
            or (l5bool9z) and (l5var14z.i >= 2) then {
            addToInsnList(KUTM+SP + l5var13z.i);
        };
    };
```

- Fortran routine: optionally emits `P/MF` (helper 92) which marshals the
  result/frame.
- Pascal routine: emits `KUTM,SP,N` to bump the stack pointer by
  `N = 3/4/5/6` words **only if** there are ≥ 2 actual args (direct) or any
  args at all (indirect). One-arg calls write directly without bumping first.

`l5bool8z` is the **"first-actual-not-yet-stored"** flag: when true, the first
computed argument is left in the accumulator (no `mcPUSH`); subsequent ones
get `mcPUSH`ed onto the stack.

## 3. Argument loop

```pascal
(loop)
    while l5exp1z <> NIL do {
        l5exp2z := l5exp1z@.expr2;
        l5exp1z := l5exp1z@.expr1;
        l5op21z := l5exp2z@.op;
        l5var14z.i := l5var14z.i + 1;
        l5inl20z := insnList;
        if (l5op21z = PCALL) or (l5op21z = FCALL) then {
```

For each actual argument:

- `l5exp2z` is the **argument's expression** (`expr2`); `l5exp1z` advances down
  the chain (`expr1`).
- `l5inl20z` saves the *current* instruction list so the per-arg code can be
  spliced in below it.

Two big sub-cases: the actual is itself a **routine name being passed**
(`PCALL`/`FCALL`) — typical for `procedure(p)` where `p` is a routine — or a
normal **value/lvalue expression**.

### 3a. Routine-as-argument: PCALL / FCALL

```pascal
            l5idr4z := l5exp2z@.id2;
            new(insnList);
            insnList@.head := NIL;
            insnList@.next := NIL;
            insnList@.regsused := [];
            set145z := set145z + l5idr4z@.flags;
            if (l5idr4z@.list <> NIL) then {
                addToInsnList(l5idr4z@.offset + insnTemp[XTA] +
                              l5idr4z@.value);
                if (l5bool10z) then
                    addToInsnList(getHelperProc(19)); (* "P/EA" *)
            } else
```

If the passed-in routine is itself a **formal parameter** (`list <> NIL`),
just load its closure word with `KXTA, offset[base]` and (for Fortran) call
`P/EA` (helper 19) to convert it.

Else the passed-in routine is a **real local/global procedure**. Block `(a)`
decides how to materialise a closure for it:

```pascal
(a)         {
                if (l5idr4z@.value = 0) then {
                    if (l5bool10z) and (21 in l5idr4z@.flags) then {
                        addToInsnList(allocGlobalObject(l5idr4z) +
                                      (KVTM+I14));
                        addToInsnList(KITA+14);
                        exit a;
                    } else {
                        l5var16z.i := 0;
                        formJump(l5var16z.i);
                        padToLeft;
                        l5idr4z@.value := moduleOffset;
                        ...
                        formAndAlign(getHelperProc(62)); (* "P/BP" *)
                        ...
                        form2Insn(
                            getHelperProc(63(*P/B6*)) + 6437777777300000C,
                            allocGlobalObject(l5idr4z) + KUJ);
                        if (l5idr3z <> NIL) then {
                            repeat
                                paramClass := l5idr3z@.cl;
                                if (paramClass = ROUTINEID) and
                                   (l5idr3z@.typ <> NIL) then
                                    paramClass := ENUMID;
                                form2Insn(0, ord(paramClass));
                                l5idr3z := l5idr3z@.list;
                            until (l5idr4z = l5idr3z);
                        };
                        storeObjWord([]);
                        P0715(0, l5var16z.i);
                    }
                };
                addToInsnList(KVTM+I14 + l5idr4z@.value);
                if 21 in l5idr4z@.flags then
                    addToInsnList(KITA+14)
                else
                    addToInsnList(getHelperProc(64)); (* "P/PB" *)
            };
```

When the routine has no helper-thunk yet (`value = 0`), `genEntry` actually
**synthesizes a thunk in-line**:

1. Jump *over* the thunk (`formJump`), saving the jump's fix-up at `l5var16z`.
2. Emit a thunk at `moduleOffset` consisting of `P/BP` (helper 62) call wrapped
   by setup of I10/I9/I8 (return point, "is function" flag, magic `74001B`),
   followed by `P/B6` (helper 63) and a `KUJ` to the actual routine.
3. After the thunk, emit one descriptor word per **formal parameter** of
   `l5idr4z`, encoding each formal's `cl` (idclass) — `ROUTINEID` for
   nested-routine results is folded down to `ENUMID`.
4. Terminate with `storeObjWord([])` and patch the forward jump.

Then emit `KVTM,I14,thunk_addr` and either `KITA+14` (Fortran) or `P/PB`
(helper 64) to push the thunk address as an argument.

After the block, the idclass for type-tag purposes is `ROUTINEID` (procedure
passed) or `ENUMID` (function passed):

```pascal
            if (l5op21z = PCALL) then
                paramClass := ROUTINEID
            else
                paramClass := ENUMID;
```

### 3b. Normal expression argument

```pascal
        } else {
            genFullExpr(l5exp2z);
            if (insnList@.ilm = il1) then
                paramClass := FORMALID
            else
                paramClass := VARID;
        };
```

`genFullExpr` emits code for the argument expression. If the result is in
**lvalue mode** (`il1` — an address), this is a "formal/by-reference" actual;
otherwise it's a plain value (`VARID`).

### 3c. Coerce to formal when the formal is by-reference

```pascal
        if not (not l5bool9z or (paramClass <> FORMALID) or
               (l5idr6z@.cl <> VARID)) then
            paramClass := VARID;
```

For a direct call, if the actual would be passed as a reference (`FORMALID`)
**but the matching formal is plain VARID**, force it to VARID (i.e. push the
*value*, not the address). The condition is the de Morgan negation:
`l5bool9z and paramClass = FORMALID and l5idr6z@.cl = VARID`.

### 3d. Materialise the actual in the accumulator

```pascal
(loop)      if (paramClass = FORMALID) or (l5bool11z) then {
            setAddrTo(14);
            addToInsnList(KITA+14);
        } else if (paramClass = VARID) then {
            if (insnList@.typ@.size <> 1) then {
                paramClass := FORMALID;
                goto loop;
            } else {
                prepLoad;
            }
        };
```

- Formal (by-reference) or routine-passes-all-by-name (`l5bool11z`): compute
  the address into I14 (`setAddrTo(14)`) and convert it to accumulator
  (`KITA+14`).
- Value with **size 1**: just `prepLoad` (normal accumulator load).
- Value with **size > 1**: cannot fit in the accumulator, so fall back to
  formal (address) mode; `goto loop` restarts the materialisation as
  `FORMALID`.

### 3e. Push and splice

```pascal
        if not l5bool8z then
            addxToInsnList(macro + mcPUSH);
        l5bool8z := false;
        if (l5inl20z@.next <> NIL) then {
            l5inl20z@.next@.next := insnList@.head;
            insnList@.head := l5inl20z@.head;
        };
        insnList@.regsused := insnList@.regsused + l5inl20z@.regsused;
        if not l5bool9z then {
            curVal.cl := paramClass;
            addToInsnList(KXTS+I8 + getFCSToffset);
        };
        if l5bool9z and not l5bool11z then
            l5idr6z := l5idr6z@.list;
    }; (* while -> 7061 *)
```

- Push the just-loaded actual onto the stack (`mcPUSH`) — except for the
  *very first* one, which stays in the accumulator (the `l5bool8z` flag).
- Splice the new arg's instruction list **in front of** the previous list (so
  arguments end up evaluated in source order even though the chain was walked
  backwards), and union their `regsused` sets.
- **Indirect call only**: also emit a `KXTS,I8,FCSToffset` storing the `cl`
  (idclass) tag word, so the callee can introspect each actual at run time.
- Advance the formal-list pointer for direct calls (unless every parameter is
  pass-all-by-name).

## 4. Emitting the call

Fortran prologue inversion (a `KNTR+2` instruction with mode 4) for
arithmetic-mode handshake:

```pascal
    if l5bool10z then {
        addToInsnList(KNTR+2);
        insnList@.next@.mode := 4;
    };
```

Direct vs indirect call:

```pascal
    if l5bool9z then {
        addToInsnList(allocGlobalObject(l5idr5z) + (KVJM+I13));
        if (20 in l5idr5z@.flags) then {
            l5var17z.i := 1;
        } else {
            l5var17z.i := l5idr5z@.offset div 4000000B;
        }
    } else {
        l5var15z := 0;
        if (l5var14z.i = 0) then {
            l5var17z.i := l5var13z.i + 1;
        } else {
            l5var17z.i := -(2 * l5var14z.i + l5var13z.i);
            l5var15z := 1;
        };
        addInsnAndOffset(macro+16 + l5var15z,
                         getValueOrAllocSymtab(l5var17z.i));
        addToInsnList(l5idr5z@.offset + insnTemp[UTC] + l5idr5z@.value);
        addToInsnList(macro+18);
        l5var17z.i := 1;
    };
    insnList@.next@.mode := 2;
```

**Direct call** (`l5bool9z`):
- `KVJM,I13, addr` — BESM-6 subroutine call, return address into I13.
- `addr` comes from `allocGlobalObject`, which lazily reserves a symbol-table
  slot for an extern (`P/`-name) or a local label.
- `l5var17z` becomes the callee's nesting level (1 for external, else
  extracted from `offset div 4000000B` which encodes the lexical level in the
  high bits).

**Indirect call** (formal/closure):
- Emit macro `macro+16 + flag, count`. `flag = 1` means "with argument-count
  word"; the count is either `+(frame_size+1)` for no-arg calls or
  `-(2*nargs + frame_size)` for n-arg calls (sign bit distinguishes the macro
  variant).
- Then `KUTC, offset[base] + value` — load the closure's code address with
  base+offset.
- Then `macro+18` — the actual indirect jump macro.
- Nesting level of the indirectly called routine is assumed 1.

`mode := 2` on the call instruction tags it as a control-transfer for the
peephole pass.

## 5. Frame/display register restore

```pascal
    if (curProcNesting <> l5var17z.i) then {
        if not l5bool10z then {
            if (l5var17z.i + 1 = curProcNesting) then {
                addToInsnList(KMTJ+I7 + curProcNesting);
            } else {
                l5var15z := frameRestore[curProcNesting][l5var17z.i];
                if (l5var15z = (0)) then {
                    curVal.i := 6017T; (* P/ *)
                    l5var19z.i := curProcNesting + 16;
                    besm(ASN64-30);
                    l5var19z := ;
                    l5var18z.i := l5var17z.i + 16;
                    besm(ASN64-24);
                    l5var18z := ;
                    curVal.m := curVal.m + l5var19z.m + l5var18z.m;
                    l5var15z := allocExtSymbol(extSymMask);
                    frameRestore[curProcNesting][l5var17z.i] := l5var15z;
                };
                addToInsnList(KVJM+I13 + l5var15z);
            }
        }
    };
```

After the call returns, the **display registers** (one per lexical level) the
callee may have trashed must be restored, because the caller is at a *deeper*
nesting level than the callee. Skipped for Fortran calls.

- One level off: a single `KMTJ,I7,curProcNesting` re-binds the current frame
  register.
- Otherwise call a small helper `P/<callerLevel><calleeLevel>` (e.g. `P/57`),
  built on the fly the first time it is needed: the routine name is assembled
  from the literal `'P/'`, the caller's level packed via `ASN64-30`, and the
  callee's level packed via `ASN64-24`. The symbol is registered with
  `allocExtSymbol` and cached in `frameRestore[curProcNesting][l5var17z.i]`
  so subsequent calls reuse the same helper.

## 6. Stack/result cleanup

```pascal
    if not l5bool9z or ([20, 21] * l5var12z.m <> []) then {
        addToInsnList(KVTM+40074001B);
    };
    set145z := (set145z + l5var12z.m) * [1:15];
    if l5bool10z then {
        if (not checkFortran) then
            addToInsnList(KNTR+7)
        else
            addToInsnList(getHelperProc(93));    (* "P/FM" *)
        insnList@.next@.mode := 2;
    } else {
        if not l5bool7z then
            addToInsnList(KXTA+SP + l5var13z.i - 1);
    };
    if not l5bool7z then {
        insnList@.typ := l5idr5z@.typ;
        insnList@.regsused := insnList@.regsused + [0];
        insnList@.ilm := ilVALINACC;
        set146z := set146z - l5var12z.m;
    }
```

- Indirect or external/Fortran (bits 20, 21): emit `KVTM,40074001B` — a
  literal-encoded helper to fix up the global stack-top register.
- Update `set145z` (caller's clobbered-register set) with the callee's flags,
  restricted to bits 1..15.
- Fortran epilogue: either `KNTR+7` (resume normal arithmetic) or `P/FM`
  (helper 93) when `checkFortran` mode is on.
- **Function** (`not l5bool7z`): `KXTA, SP+frameSize-1` pulls the return value
  off the stack into the accumulator. The result `typ` is set to the routine's
  return type, `regsused` records bit 0 (accumulator used), and `ilm = ilVALINACC`
  declares an rvalue available.

`set146z := set146z - l5var12z.m` clears bits in `set146z` that the callee was
known to depend on, because the call invalidates any prior promises about
those registers.

## Nested helper `allocGlobalObject`

```pascal
function allocGlobalObject(l6arg1z: irptr): integer;
{
    if (l6arg1z@.pos = 0) then {
        if (l6arg1z@.flags * [20, 21] <> []) then {
            curVal := l6arg1z@.id;
            curVal.m := leftAlign;
            l6arg1z@.pos := allocExtSymbol(extSymMask);
        } else {
            l6arg1z@.pos := symTabPos;
            putToSymTab([]);
        }
    };
    allocGlobalObject := l6arg1z@.pos;
};
```

Lazily assigns a symbol-table slot to a routine the first time `genEntry`
needs its address. External/Fortran routines (bits 20/21) get a named extern
symbol via `allocExtSymbol`; ordinary routines get an empty placeholder word
`putToSymTab([])` whose location becomes the call target — later patched when
the routine is actually compiled (`programme`/`defineRoutine` fills it in).

## Summary of the emitted shape

For a normal Pascal call `f(a, b, c)` with `f` a direct function:

```
KUTM,SP, frameSize           ; if ≥2 args
<eval a; first stays in ACC>
mcPUSH
<eval b>
mcPUSH
<eval c>
KVJM,I13, allocGlobalObject(f)   ; the call
KMTJ,I7, curProcNesting          ; or P/XY thunk
KVTM,40074001B                   ; if external
KXTA,SP, frameSize-1             ; fetch return value
```

For a call through a formal parameter `p(...)` you instead get the
macro+16/macro+18 indirect sequence and the routine-class words
`KXTS,I8, off` per actual, so the callee can dispatch on the runtime type tags.

That is `genEntry` end-to-end: it walks the argument chain, materialises each
actual according to the formal's kind and the call style
(Fortran / direct / indirect), emits the BESM-6 call, restores the display
registers across the lexical-level mismatch, and (for functions) leaves the
result in the accumulator with an `ilVALINACC` instruction list ready to be consumed
by the surrounding expression.
