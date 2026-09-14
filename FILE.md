# Pascal FILE Record Layout (BESM-6)

Layout of the per-file control block reached via index register **M12** by the
runtime helpers, now `libc/c_co.madlen`, `c_it.madlen`, `c_gf.madlen`,
`c_pf.madlen`, `c_tf.madlen`, `c_rf.madlen`, `c_woln.madlen` (`C/CO`, `C/IT`,
`C/GF`, `C/PF`, `C/TF`, `C/RF`, `C/WL`/`C/WOLN`). All offsets below are in
**decimal**; the parallel octal column shows the literal that appears in the
sources (`12, ATX ,nnB`). Every field's offset and role below is verified
against the current `libc/*.madlen` code; the `*NNNNB` addresses cited in
individual field descriptions are `paslib/p_sys.asm`-internal labels the
`libc/` ports were checked against and no longer correspond to addresses in
the current sources, which use their own symbolic labels instead (e.g. the
port of the code once at `p_sys.asm`'s `*0537B` is `libc/c_pf.madlen`'s
`ERRPUT`) — they remain useful only as a cross-reference into that historical
file, not as a location in the current one.

## Runtime modules

The file runtime now lives one routine to a module in `libc/` (`.madlen`
sources), ported instruction-for-instruction from the historical
`paslib/p_sys.asm` reconstruction of library 22's combined `P/SYS`. Each
`libc/` module's own header comment names the `p_sys.asm` label it was
ported from; those are carried into the table below as the historical
cross-reference. Integer modulo/divide (`C/MD`/`C/DI`) are a separate pair
of standalone helpers, not part of the `P/SYS` file-I/O reconstruction —
`libc/04-cmd.madlen` and `libc/03-cdi.madlen` implement them directly, each
its own `,NAME,` module.

| file | module | entry points | role | ported from (`p_sys.asm`) |
| --- | --- | --- | --- | --- |
| `libc/04-cmd.madlen` | `C/MD` | — | integer remainder (sign of dividend) | — (standalone, not `p_sys.asm`) |
| `libc/03-cdi.madlen` | `C/DI` | — | integer division, truncating toward zero | — (standalone, not `p_sys.asm`) |
| `libc/c_it.madlen` | `C/IT` | — | indirect-tail return (compiler helper #42) | — |
| `libc/c_co.madlen` | `C/CO` | `C/RE1`, `C/RE2` | create / open / reset a file | — |
| `libc/c_ctrp.madlen` | `C/CTRP` | `C/GT`, `C/CT`, `C/ZI` | track allocation and zone I/O | `GETTRACK`, `CHKTRACK`, `ZONEIO` |
| `libc/c_gf.madlen` | `C/GF` | `C/RACPAK`, `C/UP` | get an element; unpack the window | `C/UP` = `*0667B` |
| `libc/c_ad.madlen` | `C/AD` | — | advance the buffer iterator | `ADVANCE` |
| `libc/c_gl.madlen` | `C/GL` | — | refill the input line from stdin | `READLINE` |
| `libc/c_inbuf.madlen` | `C/INBUF` | — | allocate / refill the input buffer | — |
| `libc/c_givep.madlen` | `C/GIVEP` | — | flush a partial packed-output word | `FLUSHBUF` |
| `libc/c_pk.madlen` | `C/PK` | — | pack ACC into the buffer slot | `PACKBUF` |
| `libc/c_pf.madlen` | `C/PF` | `C/OB`, `C/RS` | put an element; flush the buffer to disk | `C/OB` = `*0642B`, `C/RS` = `*0632B` |
| `libc/c_woln.madlen` | `C/WOLN` | `C/WL`, `C/FL`, `PUTLN` | end an output line; print a record | `C/WL` = `FLUSHLIN`, `C/FL` = `*0600B` |
| `libc/c_rf.madlen` | `C/RF` | `C/OR` | reset for reading; pre-reset flush | `C/OR` = `OUTRESET` |
| `libc/c_tf.madlen` | `C/TF` | `C/OI`, `C/CB` | rewrite; open input; close the window | `C/OI` = `OPENIN`, `C/CB` = `CLOSEWIN` |
| `libc/c_ah.madlen` | `C/AH` | — | fatal-error abort handler | `ABORT` |
| `libc/c_bexf.madlen` | `C/BEXF` | — | standard external-file name → designator lookup | `P/BEXF` |
| `libc/c_bx.madlen` | `C/BX`, `C/EN` | — | build the initial stack frame and the `M1` constant block | `P/BX`/`P/EN` (`paslib/p_bx.asm`) |
| `libc/c_trpage.madlen` | `C/TRPAGE` | — | seed the disk/drum zone free-list | `P/TRPAGE` |
| `libc/c_pages.madlen` | `C/PAGES` | — | report available drum pages, leave `M9` on `P/FIRP` | `P/PAGES` |
| `libc/c_pampam.madlen` | `C/PAMPAM` | — | generic word block-copy | `P/PAMPAM` |

`PUTLN` is the address `C/WOLN` resolves to for the P2C runtime declaration
`void putln() assembler` — the spelling `NEWLINE` cannot serve, since that
name is an entry of `FTNPRT` in library 21, which the loader searches ahead
of `libc`.

Two `,LC,` commons carry the constants that cross a module boundary, both
seeded by the `,DATA,` image in `libc/c_ctrp.madlen`: `C*LANE*` (the
`0o76000` buffer/lane mask) and `C*IOBIT` (the `FILE[23]` stdin/stdout tag
bit).

| Off | Oct | Use                                       | Set by / Used by |
|----:|----:|-------------------------------------------|------------------|
|  0  |  0  | In-buffer element cursor                  | C/CO init = SP; C/GF/C/PF advance with `12, ARX ,21B` (+[17] words); compared to [1] via `12, AEX ,1` to detect window exhaustion |
|  1  |  1  | End-of-window sentinel (limit for [0])    | C/CO init from buffer layout; compared via `12, AEX ,1` to detect buffer exhaustion |
|  2  |  2  | EOF / pending flag                        | C/CO clears; C/GF returns without advancing if non-zero; C/PF aborts "PUT(F) NOT AT EOF" if zero (`UZA` at *0537B*); `P/EO`/`feof` return this field unchanged (0 = not EOF, non-zero = EOF) |
|  3  |  3  | Mode/state byte; in the disk subsystem also the file's track-table descriptor / current track id | `1, ATX ,3` early in C/CO; set to a track id by GETTRACK / CLOSEWIN / OPENIN; conditionally zeroed at *0141B |
|  4  |  4  | Open mode: input(0)/output(non-0)         | Checked by C/RE1, C/CTRP, C/GF, C/PF, C/RF, C/TF |
|  5  |  5  | Buffered I/O: within-window bit-shift / step counter. Disk subsystem: current zone / track-entry cursor | Buffered: `12, XTA ,5` … `AEX ,17B` (compare with FILE[15]), reset to FILE[15] on wrap. Disk: holds the track id / zone, used as an address via `12, WTC ,5` in GETTRACK / ZONEIO / CLOSEWIN |
|  6  |  6  | Working buffer descriptor / lane mask     | Set to `7 6000` (constant *0760B) by C/RF/C/TF; stacked by C/INBUF and C/PF |
|  7  |  7  | Buffer slot index for `WTC`               | `12, WTC ,7` sets the working tag for the next memory access at the current element slot; updated by `ADVANCE` in the text path |
|  8  | 10  | `f^` staging / last value                 | Holds the caller's `f^` word during stdin `READLINE`; XOR'd with [9] in C/GF text path; also written `1U` at open/reset to mark `f^` valid |
|  9  | 11  | Packed-element bit counter                | OR'd with caller's bit pattern in C/PF; copied with [10] in C/RE2 |
| 10  | 12  | Twin of [9] (input shadow)                | Written together with [9] at *0210B in C/RE2 |
| 11  | 13  | Buffer-window upper bound for M14         | Read at *0563B (C/PF inner loop) into M14 |
| 12  | 14  | Original buffer start pointer (`read* buf ptr`) | C/CO init = SP; compared to [19] in C/RF *0710B to detect empty buffer; saved across calls |
| 13  | 15  | Buffer end pointer (for window/limit)     | C/CO init = SP; updated by C/GF/C/PF as window advances |
| 14  | 16  | Packed-mode flag from open                | `12, XTA ,16B`+`U1A` selects packed branches in C/GF (*0336B*) and C/PF; cleared on some reset paths. Text dispatch in get/put also keys off [18] = 0 |
| 15  | 17  | Wrap value for [5] (max bit shift)        | `XTA 17B` + `ATX 5` to reload [5] when packed slot crosses word |
| 16  | 20  | Initial value of [6] (saved descriptor)   | Restored via `XTA 20B; ATX 6` in C/GF (*0336B*) and C/INBUF |
| 17  | 21  | Element stride in **words**               | Set in C/CO from M11 (`base.size`); consumed by `12, ARX ,21B` / `12, A+X ,21B` to advance [0] and (on put) [19]. Despite the `p_sys.asm` comment "bit-step", the instructions add whole words, not bits |
| 18  | 22  | Element width in bits (= M9 = `elSize`)   | Set in C/CO via `ITA 9; 12, ATX ,22B`; `UZA` on [18] selects the text/unpacked path in C/GF/C/PF/C/RF (see below) |
| 19  | 23  | Current buffer write cursor               | C/CO init = SP; advanced by [17] on C/PF; on C/GF packed wrap bumped by 1 word only (*0317B*); compared to [13]/[15] to detect end |
| 20  | 24  | `ASN` shift amount (= 64 − elSize)        | Default 56 from `*0104B` (text); else `64 − elSize` in C/CO. Loaded by C/RACPAK / packed-put as `12, XTA ,24B` and consumed by the following `,ASN,` |
| 21  | 25  | VLM loop-count base (= 50 − 48/elSize)    | Default −4 from `*0104B+1` (text); else `50 − 48/elSize` in C/CO. Used both as `12, XTA ,25B` (load count into ACC) and as `12, WTC ,25B` (set working tag for the next instruction in the unpack loop) |
| 22  | 26  | Negative shift seed for `ASX`             | Default = MSB constant (`[M1+21]`); else derived from elSize in C/CO. Consumed only as `12, ASX ,26B` to apply the per-element shift inside the packed unpack loop |
| 23  | 27  | I/O kind bits (bit 1 = stdin/out, bit 3 = `BREAK` arg) | C/GF *0330B*: `XTA 27B; UTC *0026B; AAX` masks bit 1; cleared by C/CO |
| 24  | 30  | EOLN / line-state flag                    | Cleared by C/CO; XOR'd with newline char in C/GF stdin path *0330B*; → branches to PASEOF |
| 25  | 31  | OR-pattern to apply after unpacking (sign/tag bits) | Cleared by C/CO; per `formFileInit` comment, "bit pattern to add after unpacking" |
| 26  | 32  | External file name (FCST literal), then the Pascal source id | C/CO stores A (the 8-char external name, or 0); `C/BEXF` maps the standard names to their `LLLLNNZZZZ` designators; the compiler later overwrites [26] with the internal Pascal id for diagnostics |
| 27  | 33  | Element/byte countdown (write capacity)   | C/CO init = caller's element count; decremented by `1, A-X ,10B` per put; underflow → error *0632B |
| 28  | 34  | unused                                    | — no references in p_sys.asm |
| 29  | 35  | unused                                    | — no references in p_sys.asm |

## Setup register convention for `C/CO`

`C/CO` (now `libc/c_co.madlen`) is reached either from the historical
per-procedure **FILEINIT** block (see `FILEINIT.md`) or from the explicit
**`fopen(fcb, sizes, name)`** helper in `libc/fopen.madlen`. Both pass the
same register tuple:

| Reg | Meaning |
|-----|---------|
| M12 | Address of this FILE record (30-word FCB; `ifcb` / `fileblk` in tests) |
| M11 | Base-type size in **words** (becomes [17], element stride) |
| M10 | `fileBufSize` (used to size the inline buffer) |
| M9  | `elSize` in **bits** (becomes [18]; 0 selects the text/unpacked path) |
| A   | External file name (FCST offset), or 0 for an internal file |

The `fopen` sizes argument packs the three size fields the way the old
`formFileInit` VTMs did:

```
sizes = (bufsize << 30) | (basesize << 15) | elsize
```

Examples: `0100010` = bufsize 1, basesize 1, elsize 8 (stdin/stdout via
`fopenFile`); `010000100060` = bufsize 1, basesize 1, elsize 48 (test
`55_word_file_ops`).

The current compiler's `formFileInit` only emits `fclose` calls at procedure
exit; new files are opened with `fopen` before `reset`/`rewrite`.  After a
successful open the caller may overwrite [26] with a diagnostic source-name
word (the old FILEINIT loop did this with `KATX+I12+26`).

`libc/fopen.madlen` and `libc/fclose.madlen` each carry a comment claiming
their `vjm,C/co` / `uj,C/it` tail is "commented out below" pending a linkable
`C/CO`/`C/IT`; that comment is stale — `libc/c_co.madlen` and `libc/c_it.madlen`
now exist and the tail calls are live, uncommented code in both files, so
`fopen`/`fclose` do reach `C/CO`/`C/IT` as described above.

## Per-call register convention

For every call to `C/GF`, `C/PF`, `C/TF`, `C/RF`:

- **M12** = FILE record base
- **M1**  = caller's local-data base (parameters / `f^` location at `[M1+8]`)
- **M13** = return address (preserved via `ITA 13`/`15, ATX,`)

## Element stride ([17]) vs element width ([18])

These two fields are set independently at open time and serve different roles:

| Field | Source | Role |
|-------|--------|------|
| [17] | M11 = `base.size` in words | Buffer layout alignment; per-element cursor step for [0] and (on put) [19] |
| [18] | M9 = `elSize` in bits | Text vs packed dispatch; packed shift/unpack constants |

**Text / unpacked path ([18] = 0).**  C/GF (*0345B*) and C/PF advance [0] by
[17] words per `get`/`put`.  `C/RE2` divides the buffer word count by [17] to
seed [9] as an element counter.  This is the path intended for multi-word
elements stored as contiguous whole words in the disk buffer.

**Packed path ([18] ≠ 0).**  `C/CO` derives [20] = `64 − elSize`, [21] =
`50 − 48/elSize`, and [22] from `48 mod elSize` (requires `1 ≤ elSize ≤ 48`).
`C/RACPAK` and `PACKBUF` move one `elSize`-bit field per call; they do not
loop [17] times.  Sub-word packing within a 48-bit word additionally uses [5],
[7], and [15] (wrap for [5]).

### When [17] > 1

| Mode | Behaviour |
|------|-----------|
| [18] = 0 (unpacked) | Coherent: every get/put skips [17] words; buffer sizing and [9] counts are in elements, not raw words |
| [18] ≠ 0 (packed) | Partial / inconsistent: [0] advances by [17] words, but `C/RACPAK`/`PACKBUF` still handle only one [18]-bit field; on window wrap C/GF bumps [19] by **1** word (*0317B*) while C/PF bumps [19] by **[17]** words — so packed multi-word elements are not supported end-to-end |
| [18] > 48 | Broken: `48 / elSize` is 0 in `C/CO` and the packed constants are wrong |

In this codebase all scalars have `psize = 1` and current `fopen` call sites
pass `basesize = 1`; the exercised case is [17] = 1 with [18] ≤ 48.

## Notes on packed-mode iteration

In packed mode ([18] ≠ 0; [14] may also be non-zero from open-time setup):

1. `[0]` is the in-buffer cursor; each packed `get`/`put` does
   `[0] += [17]` (words) and compares to [1].
2. When several packed values share one 48-bit word, [5] tracks the
   within-word bit shift; when it equals [15] (wrap) the logic advances via
   `ADVANCE` / the next word.
3. `[20]`, `[21]`, `[22]` are **plain numeric values** (an `ASN` shift amount,
   a VLM loop count, and an `ASX` shift seed respectively), **not** patched
   instruction templates. They are computed once in `C/CO` from [18] only
   (with text-mode defaults `56` / `−4` / MSB-constant pulled from
   `*0104B`/`[M1+21]` when [18] = 0).  They are consumed verbatim by `XTA`,
   `WTC`, and `ASX` inside `C/RACPAK` and `PACKBUF`.  There is **no
   self-modifying code** in `p_sys.asm`: every `WTC` only sets the working
   tag for the *next* instruction.

## External file designator and disk I/O

A file's external **designator** is an octal word of the form `LLLLNNZZZZ`
(each letter one octal digit) that says where the file lives on disk:

- `LLLL` = file length,
- `NN`   = logical unit (device) number,
- `ZZZZ` = starting zone number on that unit.

The FCST literal the compiler passes to `C/CO` (and that lands in `FILE[26]`)
is the file's 8-char external **name**, *not* the designator. `C/CO` stashes
that name in `FILE[26]` and scratch `[M1+3]`; for the standard files
(`STDOUT`, `STDIN`, `PASINPUT`, `*RESULT*`, `*CHILD*`) `C/BEXF`
(`libc/c_bexf.madlen`) maps the name to its `LLLLNNZZZZ` designator and the FCST decoder
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
- Bit-by-bit layout of `[23]` beyond bit 1: `libc/c_co.madlen`'s `STDFLAV`
  path sets it to `2` (`IOBIT`) for `STDOUT` and `3` (`TESTB3`) for `STDIN` —
  both carry bit 1 ("is standard input/output"), and `STDIN` additionally
  sets bit 0, which nothing observed elsewhere in `libc/` reads back; its
  purpose beyond distinguishing the two standard files is still unconfirmed.
- Original purpose of `[28]`/`[29]` — possibly reserved for an extension never
  used by the released runtime.
- Whether the text/unpacked path ([18] = 0, [17] > 1) copies all [17] words
  to/from `[M1+8]` (`f^`) on each get/put, or relies on `f^` pointing into
  the buffer — not fully traced in the current compiler.
- What a correct packed multi-word design would require (likely [17] = 1 with
  [18] = total bit width ≤ 48, or a `C/RACPAK` loop over [17]).

## The M1 block (`P/1D`)

`M1` points at the runtime block `P/1D` (declared in `libc/c_bx.madlen`,
`,LC,40` = 32 words, offsets 0..31 decimal). Offsets 6..22 are read-only
**constants** seeded at load time from the static image in `c_bx.madlen`
(image word *k* lands at `[M1+6+k]`); the rest are runtime **variables** —
frame/divide scratch and the heap / disk-allocator state. Offsets are shown
in decimal and octal.

| Dec | Oct | Contents | Kind / set by |
|----:|----:|----------|---------------|
| 0–2 | 0–2 | frame setup / link scratch (`[1]` = P/EN save-tail addr) | variable (P/BX, P/EN) |
| 3   | 3   | scratch: FCST literal, drum descriptor, divide operand | variable (scratch) |
| 4   | 4   | scratch | variable (scratch) |
| 5   | 5   | scratch | variable (scratch) |
| 6   | 6   | Load-time `0` (first word of the constant image) | constant |
| 7   | 7   | `000000` — six `'0'` chars (number-format fill) | constant |
| 8   | 10  | `1U` (the 1-bit unit for `ARX`/`AOX`/`AEX`) | constant |
| 9   | 11  | integer exponent / tag mask (bits 0,1,3) | constant |
| 10  | 12  | multiplication mask | constant |
| 11  | 13  | MAXREAL (all ones without the sign bit) | constant |
| 12  | 14  | positive-mantissa mask (bits 7..47) | constant |
| 13  | 15  | real `1.0e-6` | constant |
| 14  | 16  | real `1.0` | constant |
| 15  | 17  | minus 1 (for `AVX`) | constant |
| 16  | 20  | `77777B` (15-bit address mask) | constant |
| 17  | 21  | integer `1` | constant |
| 18  | 22  | chars `\0\7\7\7\7\7` | constant |
| 19  | 23  | real `0.5` | constant |
| 20  | 24  | all-ones (`~0U`) | constant |
| 21  | 25  | MSB, bit 48 | constant |
| 22  | 26  | mantissa without the 6 low bits | constant |
| 23  | 27  | **HEAPPTR** — heap bump pointer | variable (load-init `0o76000`; `P/GD` sets = SP) |
| 24  | 30  | **HEAPLIM** — heap overflow sentinel (`~SP`) | variable (`P/GD`) |
| 25  | 31  | **FREELST** — heap free-list head | variable (`P/NW`/`P/DS`) |
| 26  | 32  | **HEAPBSE** — heap base | variable (saved by `P/GD`) |
| 27  | 33  | head of `longjmp`'s saved-context chain | variable (`libc/longjmp.madlen`'s shared `ctxpop` tail, also entered as `C/RC`, reads/pops/writes it directly: `1, a-x ,33B` / `1, wtc ,33B` / `1, atx ,33B`) |
| 29  | 35  | packed-output cursor cache | variable (scratch, `FLUSHBUF`) |
| 30  | 36  | packed-output working pointer | variable (scratch, `FLUSHBUF`) |
| 31  | 37  | disk track free-list head | variable (built by `P/TRPAGE`; used by `GETTRACK`/`ZONEIO`) |

Offset 28 (oct 34) is not referenced by the runtime. Offsets 6..22 are the
only cells the compiler's FCST-offset optimization (`findLit`/
`getFCSToffset` in `base.cc`, `ZERO`/`E1` in `work.p2c`) may fold a literal
reference into, so they must stay true read-only constants; offset 27 was
picked for `longjmp`'s mutable chain head specifically because it falls
outside that range.
