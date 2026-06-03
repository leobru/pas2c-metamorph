# Pascal FILE Record Layout (BESM-6)

Layout of the per-file control block reached via index register **M12** by the
runtime helpers (`P/CO`, `P/IT`, `P/GF`, `P/PF`, `P/TF`, `P/RF`, `P/WL`,
`P/WOLN`). All offsets below are in **decimal**; the parallel octal column shows
the literal that appears in `p_sys.dtran` (`12, ATX ,nnB`).

| Off | Oct | Use                                       | Set by / Used by |
|----:|----:|-------------------------------------------|------------------|
|  0  |  0  | `f^` pointer / packed bit position        | P/CO init = SP; P/GF/P/PF read, OR with [17], store back, compare to [1] |
|  1  |  1  | End-of-window sentinel (limit for [0])    | P/CO init = SP+6; compared via `12, AEX ,1` to detect buffer exhaustion |
|  2  |  2  | "f^ valid / pending" flag (destination addr) | P/CO clears; P/PF errors if 0 (nothing to put); P/GF errors if non-0 (read already pending); P/GF success path sets it to caller's destination |
|  3  |  3  | Mode/state byte (cleared in P/CO low path)| `1, ATX ,3` early in P/CO; conditionally zeroed at *0141B |
|  4  |  4  | Open mode: input(0)/output(non-0)         | Checked by P/RE1, PASCTRP, P/GF, P/PF, P/RF, P/TF |
|  5  |  5  | Running bit shift / packed step counter   | `12, XTA ,5` … `AEX ,17B` (compare with FILE[15]); reset to FILE[15] when wraps |
|  6  |  6  | Working buffer descriptor / lane mask     | Set to `7 6000` (constant *0760B) by P/RF/P/TF; stacked by PASINBUF and P/PF |
|  7  |  7  | Current element index used as `WTC` operand | `12, WTC ,7` for self-modifying access to packed slot; updated on each step |
|  8  | 10  | Current data word being assembled / read  | Loaded from `[M1+8]` (caller value) or stored back to caller's variable |
|  9  | 11  | Packed-element bit counter                | OR'd with caller's bit pattern in P/PF; copied with [10] in P/RE2 |
| 10  | 12  | Twin of [9] (input shadow)                | Written together with [9] at *0210B in P/RE2 |
| 11  | 13  | Buffer-window upper bound for M14         | Read at *0563B (P/PF inner loop) into M14 |
| 12  | 14  | Original buffer start pointer (`read* buf ptr`) | P/CO init = SP; compared to [19] in P/RF *0710B to detect empty buffer; saved across calls |
| 13  | 15  | Buffer end pointer (for window/limit)     | P/CO init = SP; updated by P/GF/P/PF as window advances |
| 14  | 16  | Packed-mode flag (0 = text, ≠ 0 = packed) | `12, XTA ,16B`+`U1A` selects packed iteration path in P/GF (*0336B*) and P/PF |
| 15  | 17  | Wrap value for [5] (max bit shift)        | `XTA 17B` + `ATX 5` to reload [5] when packed slot crosses word |
| 16  | 20  | Initial value of [6] (saved descriptor)   | Restored via `XTA 20B; ATX 6` in P/GF (*0336B*) and PASINBUF |
| 17  | 21  | Bit step / increment per packed element   | Set in P/CO from M11 (= `l4var2z->size`); used in `12, ARX ,21B` to advance [0] and [19] |
| 18  | 22  | Element width in bits (= M9 = `elSize`)   | Set in P/CO via `ITA 9; 12, ATX ,22B`; checked for 0 (text vs binary) in P/GF/P/PF/P/RF |
| 19  | 23  | Current buffer pointer (packed write cur.) | P/CO init = SP; OR'd with caller index in P/PF / P/GF; XOR'd with [13]/[15] to detect end |
| 20  | 24  | Instruction template (ATX 0) for codegen  | Loaded from *0104B in P/CO; used by P/RACPAK as runtime-modified op |
| 21  | 25  | Instruction template (ATX 70B) for codegen | Loaded from *0104B+1; companion to [20], used in `12, WTC ,25B` self-mod |
| 22  | 26  | Negative shift count for `ASN` (= -[5])   | `XTA 25B; ASN 64-20; 12, ATX ,11B; XTA 25B; 12, ATX ,26B` — derived in P/CO |
| 23  | 27  | I/O kind bits (bit 1 = stdin/out, bit 3 = `BREAK` arg) | P/GF *0330B*: `XTA 27B; UTC *0026B; AAX` masks bit 1; cleared by P/CO |
| 24  | 30  | EOLN / line-state flag                    | Cleared by P/CO; XOR'd with newline char in P/GF stdin path *0330B*; → branches to PASEOF |
| 25  | 31  | OR-pattern to apply after unpacking (sign/tag bits) | Cleared by P/CO; per `formFileInit` comment, "bit pattern to add after unpacking" |
| 26  | 32  | External file name (8-char id)            | Written by P/CO from A (= FCST literal or 0); then overwritten by compiler with internal Pascal id |
| 27  | 33  | Element/byte countdown (write capacity)   | P/CO init = caller's element count; decremented by `1, A-X ,10B` per put; underflow → error *0632B |
| 28  | 34  | unused                                    | — no references in p_sys.dtran |
| 29  | 35  | unused                                    | — no references in p_sys.dtran |

## Setup register convention for `P/CO`

The compiler (see `work.p2c` `formFileInit`, ~line 3867) sets up before calling
P/CO:

| Reg | Meaning |
|-----|---------|
| M12 | Address of this FILE record |
| M11 | Base type size (becomes [17], the bit-step per element) |
| M10 | `fileBufSize` (used to size the inline buffer; ends up as base of [13]/[1]) |
| M9  | `elSize` (becomes [18], element width in bits) |
| A   | External file name (FCST offset), or 0 for an internal file |

Immediately after P/CO returns, the compiler emits
`KATX+I12+26` to overwrite [26] with the internal Pascal identifier of the file
(the source-level variable name) — useful for diagnostics.

## Per-call register convention

For every call to P/GF, P/PF, P/TF, P/RF:

- **M12** = FILE record base
- **M1**  = caller's local-data base (parameters / `f^` location at `[M1+8]`)
- **M13** = return address (preserved via `ITA 13`/`15, ATX,`)

## Notes on packed-mode iteration

In packed mode (FILE[14] ≠ 0, i.e. `f: file of T` where T is not a text char):

1. `[0]` holds the *bit address* of the current element inside the buffer.
2. Each `get`/`put` does `[0] |= [17]; if ([0] == [1]) flush/refill`.
3. `[5]` tracks the within-word bit shift; when it equals `[15]` (wrap) the
   logic advances to the next 48-bit word using `[20]`/`[21]` as templates for
   the `ATX`/`XTA` instruction patched via `WTC`.
4. `[22]` is the negative of `[5]` (precomputed so `ASN 64-22B` is a single
   instruction needing no further setup).

## Open questions

- Exact distinction between `[3]`, `[4]` and `[10]` (all three look mode-related).
- Bit-by-bit layout of `[23]`; only bit 1 (the `& 2` mask via `*0026B`) is
  confirmed to mean "is standard input/output".
- Original purpose of `[28]`/`[29]` — possibly reserved for an extension never
  used by the released runtime.
