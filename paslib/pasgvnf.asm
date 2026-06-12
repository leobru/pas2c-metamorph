 PASGVNF:,NAME,DTRAN  /01.06.84/
C===========================================================
C PASGVNF - "Pascal Give N-th File element".
C
C Random-access GET on a Pascal file: positions f^ on the
C N-th element of file M12 and unpacks it into the user's
C destination, then returns to the caller.  Used by the
C runtime when the program writes `f^[n]`-style indexed
C access on a packed file (the helper-name table in the
C compiler does not call this entry directly; it is
C reached from a wrapper inside another runtime module).
C
C Caller convention:
C   M12  = FILE record (see FILE.md for the 30-word layout)
C   M13  = link from VJM
C   ACC  = N (zero-based element index, integer-tagged)
C   [SP] = caller-pushed scratch slot whose address ends up
C          at [M15-8] inside this routine (used as the
C          working copy of N for the bit-offset arithmetic
C          on lines starting at *0012B and *0054B).
C
C Imports: P/DIV, P/MOD, P/RF, PASGIVEP, PASCTRP, INBUF,
C          P/RACPAK, P/WOLN, P/7A, P/WI, P/HT.
C
C On range error prints
C        " PASGENF    <N> GT <FILE[9]> FILE EL NUM"
C and HALTs through P/HT.
C===========================================================
 P/DIV:,SUBP,                 . integer divide helper (untagged)
 P/MOD:,SUBP,                 . integer modulo helper
 P/RF:,SUBP,                  . reset file for reading
 PASGIVEP:,SUBP,              . flush partial packed-output word
 PASCTRP:,SUBP,               . pre-allocate disk tracks
 INBUF:,SUBP,                 . refill input buffer (= PASINBUF)
 P/RACPAK:,SUBP,              . unpack one element via FILE[20..22]
 P/WOLN:,SUBP,                . write *OUTPUT* line + newline
 P/7A:,SUBP,                  . write 6-bit alfa: M11 addr, M10 count
 P/WI:,SUBP,                  . write integer (top of stack)
 P/HT:,SUBP,                  . terminate program
C===========================================================
C Stack frame after the prologue (M15 = SP after UTM,6):
C   [SP-8] = caller-pushed N working copy (read-only here)
C   [SP-7] = saved M13 (return address; restored at *0052B)
C   [SP-6] = scratch:  initially N, then FILE[18]=elSize in
C            the text branch, or N mod (2-FILE[21]) in the
C            packed branch
C   [SP-5] = bit-offset of element N in the file (set at
C            *0012B; reused as buffer-index in *0042B)
C   [SP-4] = multiplier feeding the *= step at *0012B+5
C            (1U for text mode, 2-FILE[21] for packed)
C   [SP-3] = buffer length in words (FILE[13]-FILE[12])
C   [SP-2] = elements-per-buffer (bufLen / FILE[17] [* mul])
C   [SP-1] = unused
C   [SP+0] = scratch top, used by *0042B
C===========================================================
 ,NTR,3                       . normalize-tag mode 3
 9,VTM,*0104B                 . M9 -> small literal table
                              .   *0104B = 2
                              .   *0105B = 0o1777
                              .   *0106B = 0o76001
 1,AAX,14B                    . untag N: ACC &= [M1+12]
                              .   (positive-mantissa mask)
 ,ITS,13                      . push M13 (caller link)
 ,XTS,                        . push ACC = N (untagged)
 15,UTM,6                     . SP += 6 -> reserve scratch
                              .   slots [SP-5..SP+0]
C ---- Make sure the file is in read state -------------------
 12,XTA,2                     . ACC := FILE[2] (pending flag)
 ,UZA,10B                     . if zero -> skip the P/RF call
 13,VJM,P/RF                  . else reset/rewind file
 12,XTA,22B                   . ACC := FILE[18] (element bits)
 ,U1A,*0054B                  . non-zero -> packed-mode prelude
C ---- Text/byte branch:                                      .
C  bit-offset := N * FILE[17]; multiplier for *0012B+5 = 1U.  .
 15,ATX,-6                    . [SP-6] := 0 (= FILE[18] here)
 1,XTA,10B                    . ACC := [M1+8] = 1U
 15,ATX,-4                    . [SP-4] := 1
 15,XTA,-8                    . ACC := caller's N (working)
 1,AOX,11B                    . tag as integer
 12,A*X,21B                   . ACC *= FILE[17] (bit step)
 ,YTA,30B                     . keep low 30 bits of product
C ---- Common tail: bounds check + buffer-zone resolution ----
*0012B:15,ATX,-5              . [SP-5] := bit-offset of N
 12,XTA,15B                   . ACC := FILE[13] (buffer end)
 12,A-X,14B                   . ACC -= FILE[12] (buffer start)
                              .   = buffer length in words
 15,ATX,-3                    . [SP-3] := buffer length
 12,XTS,21B                   . push ACC, ACC := FILE[17]
 14,VJM,P/DIV                 . divide bufLen by bit-step
 1,AOX,11B                    . tag quotient
 15,A*X,-4                    . *= [SP-4] (= 1 or 2-FILE[21])
 ,YTA,30B
 15,ATX,-2                    . [SP-2] := elements-per-buffer
 12,XTA,11B                   . ACC := FILE[9] (file element ct)
 15,A-X,-8                    . ACC -= caller N
 ,U1A,*0062B                  . N >= count -> "PASGENF GT" abort
C ---- Decide which buffer-zone holds element N --------------
 15,XTA,-8                    . ACC := N
 15,XTS,-2                    . push, ACC := elemsPerBuf
 14,XTS,-2                    . push pre-result, ACC := mod
 14,VJM,P/DIV                 . zoneIndex := N / elemsPerBuf
 12,XTS,10B                   . push, ACC := FILE[8] (cur slot)
 15,XTS,-4
 14,VJM,P/DIV                 . normalise against multiplier
 15,AEX,                      . compare with current zone
 ,UZA,*0042B                  . same zone -> *0042B (just seek)
C ---- Different zone: walk the track-list -------------------
 15,XTA,-5                    . ACC := bit-offset of N
 ,ASN,-1                      . shift right 1 -> word index
 ,ATI,14                      . M14 := count of links to walk
 12,ATX,5                     . FILE[5] := count
 12,XTA,4                     . ACC := FILE[4] (open mode)
 ,UZA,*0042B                  . input file -> done, position
 12,XTA,3                     . else load FILE[3] (write track)
 12,ATX,5                     . FILE[5] := track id (start)
*0032B:14,VZM,*0035B          . M14==0 -> finish at *0035B
 12,WTC,5                     . WT := FILE[5]
 ,XTA,                        . ACC := mem[FILE[5]] = next link
 12,ATX,5                     . FILE[5] := next track
 14,UTM,-1                    . --M14
 ,UJ,*0032B                   . loop until M14 hits zero
*0035B:15,XTA,-5              . ACC := bit-offset of N
 9,AAX,1                      . AND mem[M9+1] = 0o1777
 9,A+X,2                      . ADD mem[M9+2] = 0o76001
 12,ATX,6                     . FILE[6] := lane mask + offset
 13,VJM,PASGIVEP              . flush in-flight write
 13,VJM,PASCTRP               . claim a new track
 13,VJM,INBUF                 . refill input buffer
C ---- Position FILE[19]/FILE[0] inside the chosen zone ------
*0042B:15,XTA,-5              . ACC := bit-offset of N
 15,ATX,                      . [SP] := bit-offset
 15,XTS,-5                    . push, ACC := bit-offset
 14,VJM,P/DIV                 . word index = bitOff / step
 14,VJM,P/MOD                 . within-word remainder
 12,A+X,14B                   . += FILE[12] (buffer start)
 12,ATX,23B                   . FILE[19] := slot pointer
 12,ATX,                      . FILE[0]  := slot pointer
 15,XTA,-8                    . ACC := caller N
 12,ATX,10B                   . FILE[8]  := N (-> f^ slot)
 12,XTA,22B                   . ACC := FILE[18]
 ,UZA,*0052B                  . text mode -> skip unpack
 13,VJM,P/RACPAK              . unpack the packed element
 15,A+X,-6                    . += [SP-6] (within-word adjust)
 12,ATX,                      . FILE[0]  := final cursor
C ---- Restore frame and return ------------------------------
*0052B:15,UTM,-8              . release 8 stack slots
 15,WTC,1                     . WT := mem[SP+1] (saved M13)
 ,UJ,                         . return to caller
C===========================================================
C Packed-mode bit-offset prelude.  Computes
C   [SP-4] := 2 - FILE[21]            (= 48/elSize - 48)
C   [SP-6] := N mod (2 - FILE[21])
C   [SP-5] := N / (2 - FILE[21])      (delegated to P/DIV
C            via the indirect return through M14 = *0012B)
C The *0054B operand on the first line is dtran's
C rendering of the encoded operand value 0o54; the load
C reads the constant 2 from mem[M9+0].
C===========================================================
*0054B:9,XTA,*0054B           . ACC := mem[M9+0] = 2
 12,A-X,25B                   . ACC -= FILE[21]
 15,ATX,-4                    . [SP-4] := 2 - FILE[21]
 15,XTA,-8                    . ACC := caller N
 15,XTS,-4                    . push, ACC := [SP-4]
 14,VJM,P/MOD                 . ACC := N mod (2-FILE[21])
 15,ATX,-6                    . [SP-6] := N mod ...
 15,XTA,-8                    . ACC := caller N
 15,XTS,-5                    . push, ACC := scratch
 14,VTM,*0012B                . arrange P/DIV to return to *0012B
 ,UJ,P/DIV                    . tail-jump P/DIV: ACC = N/...
                              .   then resumes common tail
C===========================================================
C Out-of-range error path.  Prints
C   " PASGENF    " <N> " GT" <FILE[9]> " FILE EL NUM"
C and halts.
C===========================================================
*0062B:13,VJM,P/WOLN          . terminate any partial OUTPUT line
 11,VTM,*0077B                . M11 -> " PASGENF    "
 10,VTM,14B                   . M10 := 12 chars
 13,VJM,P/7A                  . print 12-char header
 1,XTA,11B                    . ACC := [M1+9] = integer-tag mask
 15,XTS,-9                    . push tag, ACC := caller N (1 word
                              .   below the prologue's saved M13;
                              .   matches the layout that would
                              .   place N at [SP-9] before any
                              .   pops here)
 13,VJM,P/WI                  . write N as decimal integer
 11,VTM,*0101B                . M11 -> " GT"
 10,VTM,3                     . M10 := 3 chars (only " GT" of the
                              .   6-char *0101B word is wanted)
 13,VJM,P/7A
 1,XTA,11B                    . re-tag mask
 12,XTS,11B                   . push, ACC := FILE[9] = element ct
 13,VJM,P/WI                  . write element count
 11,VTM,*0102B                . M11 -> " FILE EL NUM"
 10,VTM,17B                   . M10 := 15 (12 chars + 3 padding
                              .   from *0104B literal table)
 13,VJM,P/7A
 13,VJM,P/WOLN                . terminate the diagnostic line
 13,VJM,P/HT                  . halt program (no return)
C===========================================================
C Diagnostic strings, 6 chars per word in BESM-6 ISO 6-bit.
C===========================================================
*0077B:,ISO,6H′40′′120′′101′′123′′107′′105′    . " PASGE"
 ,ISO,6H′116′′106′′40′′40′′40′′40′             . "NF    "
*0101B:,ISO,6H′40′′107′′124′′40′′40′′40′       . " GT   "
*0102B:,ISO,6H′40′′106′′111′′114′′105′′40′     . " FILE "
 ,ISO,6H′105′′114′′40′′116′′125′′115′          . "EL NUM"
C===========================================================
C Local literal pool, addressed via M9 = address-of(*0104B).
C===========================================================
*0104B:,LOG,2                 . M9+0: constant 2 (used as
                              .   `2 - FILE[21]` in *0054B)
 ,LOG,1777                    . M9+1: 0o1777 mask (zone-index)
 ,LOG,7 6001                  . M9+2: 0o76001 (= lane mask 76000
                              .   | bit-0); written into FILE[6]
                              .   at *0035B as the new buffer
                              .   descriptor
C===========================================================
C Trailer: small "current element index" query stub.  Returns
C   FILE[8] (the f^ slot, integer-tagged) when FILE[2] != 0,
C   FILE[9] (the file element count, integer-tagged) when
C FILE[2] == 0.  Reachable as a separate entry-by-fall-through
C from PASGVNF callers that want to read back the cursor.
C===========================================================
 12,XTA,2                     . ACC := FILE[2] (pending flag)
 ,UZA,*0112B                  . zero -> return FILE[9] instead
 12,XTA,10B                   . ACC := FILE[8] (current f^ slot)
*0111B:1,AOX,11B              . re-tag as integer
 13,UJ,                       . return through M13
*0112B:12,XTA,11B             . ACC := FILE[9] (element count)
 ,UJ,*0111B                   . tag and return
 ,END,
