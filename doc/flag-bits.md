In `identrec.flags` for `ROUTINEID`, bits `0..15` are mostly register-use/save information. Bits `20..25` are special routine attributes:

- `20`: external routine
  - Set by `EXTERN`.
  - `allocGlobalObject` emits an external symbol table entry for it.
  - Calls treat its static level as external/top-level (`l5var17z := 1`).
  - Causes post-call `KVTM+40074001B` in the normal call path.

- `21`: `FORTRAN` routine
  - Set by the `FORTRAN` designator.
  - Also treated as external by `allocGlobalObject`.
  - Enables FORTRAN call convention in `genEntry`: `P/MF`/`P/FM` when checking, `KNTR` setup/cleanup otherwise, special routine-argument handling.

- `22`: declared under `declExternal`
  - Initial routine flags become `[0:15,22]` when `declExternal` is active.
  - In `defineRoutine`, `bool48z := 22 IN procName@.flags`.
  - It suppresses the leaf/short-entry optimization that otherwise may rewrite the routine prologue and set bit `25`.

- `23`: has large by-value parameters
  - Set in `parseParameters` when by-value parameters include multiword objects.
  - In `defineRoutine`, causes startup code to copy/load long parameters via helper `P/LNGPAR`.

- `24`: all arguments by reference / checked FORTRAN mode
  - In `genEntry`, `allByRef := 24 in calleeFl.m`; every actual is passed as an address.
  - In `parseCallArgs`, `noArgs := (list = NIL) and not (24 in flags)`, so bit 24 changes how an empty formal list is interpreted.
  - Set with bit `21` for the first checked `FORTRAN` declaration.

- `25`: optimized special entry/prologue marker
  - Set in `defineRoutine` when a nested routine qualifies for a compact/leaf-style entry rewrite.
  - The code patches `objBuffer[1]` and stores `procName@.pos`; bit 25 records that this special entry form was used.

And now, from the ASSEMBLER change, bit `26` is the new assembler-routine marker.

