# Pascal FILE Record Layout (BESM-6)

Layout of the per-file control block reached via index register **M12** by the
runtime helpers (`P/CO`, `P/IT`, `P/GF`, `P/PF`, `P/TF`, `P/RF`, `P/WL`,
`P/WOLN`). All offsets below are in **decimal**; the parallel octal column shows
the literal that appears in `p_sys.asm` (`12, ATX ,nnB`).

| Off | Oct | Use                                       | Set by / Used by |
|----:|----:|-------------------------------------------|------------------|
|  0  |  0  | `f^` pointer / packed bit position        | P/CO init = SP; P/GF/P/PF read, OR with [17], store back, compare to [1] |
|  1  |  1  | End-of-window sentinel (limit for [0])    | P/CO init = SP+6; compared via `12, AEX ,1` to detect buffer exhaustion |
|  2  |  2  | "f^ valid / pending" flag (destination addr) | P/CO clears; P/PF errors if 0 (nothing to put); P/GF errors if non-0 (read already pending); P/GF success path sets it to caller's destination |
|  3  |  3  | Mode/state byte; in the disk subsystem also the file's track-table descriptor / current track id | `1, ATX ,3` early in P/CO; set to a track id by GETTRACK / CLOSEWIN / OPENIN; conditionally zeroed at *0141B |
|  4  |  4  | Open mode: input(0)/output(non-0)         | Checked by P/RE1, PASCTRP, P/GF, P/PF, P/RF, P/TF |
|  5  |  5  | Buffered I/O: within-window bit-shift / step counter. Disk subsystem: current zone / track-entry cursor | Buffered: `12, XTA ,5` … `AEX ,17B` (compare with FILE[15]), reset to FILE[15] on wrap. Disk: holds the track id / zone, used as an address via `12, WTC ,5` in GETTRACK / ZONEIO / CLOSEWIN |
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
| 20  | 24  | `ASN` shift amount (= 64 − elSize)        | Default 56 from `*0104B` (text); else `64 − elSize` in P/CO. Loaded by P/RACPAK / packed-put as `12, XTA ,24B` and consumed by the following `,ASN,` |
| 21  | 25  | VLM loop-count base (= 50 − 48/elSize)    | Default −4 from `*0104B+1` (text); else `50 − 48/elSize` in P/CO. Used both as `12, XTA ,25B` (load count into ACC) and as `12, WTC ,25B` (set working tag for the next instruction in the unpack loop) |
| 22  | 26  | Negative shift seed for `ASX`             | Default = MSB constant (`[M1+21]`); else derived from elSize in P/CO. Consumed only as `12, ASX ,26B` to apply the per-element shift inside the packed unpack loop |
| 23  | 27  | I/O kind bits (bit 1 = stdin/out, bit 3 = `BREAK` arg) | P/GF *0330B*: `XTA 27B; UTC *0026B; AAX` masks bit 1; cleared by P/CO |
| 24  | 30  | EOLN / line-state flag                    | Cleared by P/CO; XOR'd with newline char in P/GF stdin path *0330B*; → branches to PASEOF |
| 25  | 31  | OR-pattern to apply after unpacking (sign/tag bits) | Cleared by P/CO; per `formFileInit` comment, "bit pattern to add after unpacking" |
| 26  | 32  | External file name (FCST literal), then the Pascal source id | P/CO stores A (the 8-char external name, or 0); `P/BEXF` maps the standard names to their `LLLLNNZZZZ` designators; the compiler later overwrites [26] with the internal Pascal id for diagnostics |
| 27  | 33  | Element/byte countdown (write capacity)   | P/CO init = caller's element count; decremented by `1, A-X ,10B` per put; underflow → error *0632B |
| 28  | 34  | unused                                    | — no references in p_sys.asm |
| 29  | 35  | unused                                    | — no references in p_sys.asm |

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
   logic advances to the next 48-bit word.
4. `[20]`, `[21]`, `[22]` are **plain numeric values** (an `ASN` shift amount,
   a VLM loop count, and an `ASX` shift seed respectively), **not** patched
   instruction templates. They are computed once in `P/CO` from `elSize`
   (with text-mode defaults `56` / `−4` / MSB-constant pulled from
   `*0104B`/`[M1+21]`) and are then consumed verbatim by `XTA`, `WTC` and
   `ASX` instructions inside `P/RACPAK` and the packed-put path. There is
   **no self-modifying code** in `p_sys.asm`: every `WTC` only sets the
   working tag for the *next* instruction.

## External file designator and disk I/O

A file's external **designator** is an octal word of the form `LLLLNNZZZZ`
(each letter one octal digit) that says where the file lives on disk:

- `LLLL` = file length,
- `NN`   = logical unit (device) number,
- `ZZZZ` = starting zone number on that unit.

The FCST literal the compiler passes to `P/CO` (and that lands in `FILE[26]`)
is the file's 8-char external **name**, *not* the designator. `P/CO` stashes
that name in `FILE[26]` and scratch `[M1+3]`; for the standard files
(`*OUTPUT*`, `*INPUT*`, `PASINPUT`, `*RESULT*`, `*CHILD*`) `P/BEXF`
(`p_bexf.asm`) maps the name to its `LLLLNNZZZZ` designator and the FCST decoder
(`*0070B`/`*0071B`/`*0074B`) writes that back into `[M1+3]`. The decoder then
peels the open-mode / stdin bits into `FILE[3]`, `FILE[4]` and the stdin flag in
`FILE[23]`, while the designator's low 18 bits (`00NN ZZZZ`) are the unit/zone
used directly by the disk syscall. After the open, `FILE[26]` is overwritten
with the Pascal source id for diagnostics.

Packed disk files are read/written one **zone** at a time through the `*70`
supervisor call. Its one-word argument is, in octal, `00D0 PP00 00NN ZZZZ`:

- `ZZZZ` = zone (bits 1..12), `NN` = unit (bits 13..18),
- `PP`   = page (bits 31..36), `D` = 0 write / 1 read (bit 40, supplied by the
  left-packed `,OCT,001` literal `*0752B`).

Pages and zones are `02000` (1024) words and every transfer is page-aligned.
The runtime keeps a free-track table at `[M1+37B]`; `FILE[3]`/`FILE[5]` hold a
file's current track descriptor / zone cursor and `FILE[6]` the lane mask
(`0o76000`). The internal helpers driving this are `GETTRACK` (claim tracks),
`CHKTRACK` (validate the table), `ZONEIO` (issue the `*70`), `FLUSHBUF` /
`PACKBUF` (buffer ↔ disk), `CLOSEWIN` / `OUTFIN` / `OUTRESET` (window / finish)
and `OPENIN`. `READ*` is **not** a disk primitive: it reads a single stdin line
(≤80 chars) into the file's buffer and is used only by the stdin refill path
(`READLINE`).

## Open questions

- `[10]` still looks mode/shadow-related; `[3]` is the mode/state byte (also
  reused as a disk track id) and `[4]` is the read(0) / write(non-0) side.
- Bit-by-bit layout of `[23]`; only bit 1 (the `& 2` mask via `*0026B`) is
  confirmed to mean "is standard input/output".
- Original purpose of `[28]`/`[29]` — possibly reserved for an extension never
  used by the released runtime.

## Decimal offsets of useful global constants accessed via M1:

    ASCII '0' - 7
    1U        - 8
    integer exponent, bits [0,1,3] - 9
    multiplication mask - 10
    positive mantissa, bits [7..47] - 12
    minus 1 for AVX - 15
    77777U - 16
    real 1.0- 17
    real 0.5 - 19
    all one bits, ~0U - 20
    MSB, bit [0] - 21
    heap pointer - 23
