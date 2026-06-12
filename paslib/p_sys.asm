 P/SYS:,NAME,DTRAN  /01.06.84/
C===========================================================
C P/SYS - core Pascal runtime: file open/get/put/reset/close
C   and the abort handler used by all runtime fatal errors.
C
C Per-call register convention (see FILE.md):
C   M12  = base of the FILE record being acted on
C   M1   = pointer into the runtime constant table.  The
C          read-only constants the routines rely on are at
C          fixed offsets, e.g. [M1+7]='0', [M1+8]=1 (1U used
C          for AOX/ARX/AEX bit-0 work, NOT f^!), [M1+9]=
C          integer-tag mask, [M1+12]=positive-mantissa mask,
C          [M1+25B]=MSB.  The first six slots ([M1+3..+5] and
C          [M1+35B..+37B]) are runtime scratch.
C   M13  = link register set by the calling VJM
C   M14  = secondary link (used inside packed-mode helpers)
C===========================================================
 PASENDS*:,LC,1               . line-printer NEWLINE constant
 PASEOLSY:,LC,1               . end-of-line char (ASCII LF)
 PASEOFCD:,LC,1               . end-of-file sentinel record
 P/BEXF:,SUBP,                . FCST->external-name lookup
 P/DA:,SUBP,                  . print "OVERANGE IN" diagnostic
 P/TF:,SUBP,                  . forward decl, defined below
 P/MD:,SUBP,                  . integer modulo helper (P/MOD)
 P/DI:,SUBP,                  . integer divide helper (P/DIV)
 P/PF:,SUBP,                  . forward decl, defined below
 PASPMDAD:,LC,1               . post-mortem-dump entry, or 0
 STOP*:,SUBP,                 . runtime STOP / fatal exit
 SPACE*:,LC,1                 . ASCII space constant
 P/PRINT:,SUBP,               . printf-like core (in p_print)
 P/HT:,SUBP,                  . HALT - terminate the program
 PASZERO*:,LC,1               . ASCII '0' constant
 P/WOLN:,SUBP,                . forward decl, defined below
 P/WXD:,SUBP,                 . write a 48-bit word as 6-bit chars
 P/WL:,SUBP,                  . forward decl, defined below
 PASEOF:,SUBP,                . MONCARD eof-card detector
 READ*:,SUBP,                 . low-level disk-zone READ primitive
 *OUTPUT*:,LC,30              . 30-word FILE record of OUTPUT
C===========================================================
C Abort handler.  Entered by `,UJ,*0000B' with M10 pointing at
C an ,ISO, error message.  Re-seeds PASZERO* with ASCII '0'
C (it might have been used as scratch by earlier P/WXD output),
C then prints:
C   - the error text (P/PRINT reads from M11 = M10+2),
C   - the offending file's external name (FILE[26]) via P/WXD,
C     which reads the field-width parameter (= 12) that we
C     leave on the stack one slot below P/WXD's frame.
C   - a CRLF (P/WOLN -> P/WL).
C If a post-mortem-dump entry was installed at PASPMDAD it is
C called with M13 := STOP*; otherwise we jump straight to STOP*.
C===========================================================
 *0000B:,UTC,*0751B.=60     . load constant 0o60 = ASCII '0'
 ,XTA,
 ,UTC,PASZERO*              . re-seed PASZERO* (in case it moved)
 ,ATX,
 10,MTJ,11                  . M11 := M10 (caller's error-msg ptr)
 11,UTM,2                   . skip ,ISO, header words
 13,VJM,P/PRINT             . print the error string
 ,UTC,*0756B.=I12           . load constant 12 = field width
 ,XTA,
 12,XTS,32B                 . push width=12; ACC := FILE[26]=name
 15,ATX,                    . push the ext name as P/WXD argument
 13,VJM,P/WOLN              . write OUTPUT line so far
 15,XTA,                    . pop the ext name back into ACC
 13,VJM,P/WXD               . print it (8 6-bit chars, padded)
 13,VJM,P/WL                . terminate the line
 14,VTM,PASPMDAD            . M14 := &PASPMDAD
 14,XTA,                    . ACC := PASPMDAD
 ,UZA,STOP*                 . if not installed, just STOP
 13,VTM,STOP*               . arrange PMD to return to STOP*
 14,WTC,                    . WT  := PASPMDAD value
 ,UJ,                       . jump to PASPMDAD
C===========================================
 P/MOD:,ENTRY,
C===========================================
C Integer modulo, ABI-glue around P/MD.  Caller passes the two
C operands in (top-of-stack, ACC); 1,AEX,11B normalises each
C operand's tag bits ([M1+9] = integer exponent mask) so the
C divide helper sees pure integers.  Result is re-tagged before
C the indirect return through M14.
 *0014B:1,AEX,11B           . untag ACC (ACC ^= integer-exp mask)
 15,STX,2                   . save M2 on the stack
 1,AEX,11B                  . re-tag - leaves ACC's bits in place
 15,XTS,2                   . XCHG with [SP+2] = top operand
 13,VJM,P/MD                . call modulo helper
 1,AEX,11B                  . tag result back to integer
 14,UJ,                     . indirect return via M14
C===========================================
 P/DIV:,ENTRY,
C===========================================
C Integer division - identical scaffolding as P/MOD but routes
C through P/DI.
 *0020B:1,AEX,11B           . untag ACC
 15,STX,2                   . save M2
 1,AEX,11B
 15,XTS,2                   . XCHG with top operand
 13,VJM,P/DI                . call divide helper
 1,AEX,11B                  . re-tag quotient
 14,UJ,                     . indirect return
C===========================================================
C Small literal pool used by P/CO and friends.  These are
C plain data words, NOT instructions and NOT self-modifying
C templates - P/CO loads them through `9,VTM,*0024B; 9,XTA,N'
C and friends, treating them as a six-entry table.  M1 (the
C constant base) does not work for them because the values
C are needed alongside other constants only known here.
C
C   *0024B = 64    (shift base; 64 - elSize -> bit step)
C   *0025B = 48    (BESM-6 word width in bits; divisor)
C   *0026B =  2    (bit-1 mask used to test FILE[23] for
C                   "is *INPUT* / *OUTPUT*")
C   *0027B = 50    (50 - (48/elSize) -> *0513B loop count)
C   *0030B = 51    (only the dtran lists it; never loaded)
C   *0031B =  3    (test bit pattern 0o3 used at *0124B)
C===========================================================
 *0024B:,LOG,100
 ,LOG,60
 *0026B:,LOG,2
 ,LOG,62
 ,LOG,63
 *0031B:,LOG,3
C===========================================
 P/IT:,ENTRY,
C===========================================
C P/IT - "Indirect Tail" return.  Pops the two-word activation
C  frame off the BESM-6 stack and jumps to the saved link
C  address now exposed at SP+1.  Used by every runtime routine
C  that allocated a 2-word save area before doing real work.
 15,UTM,-2                  . SP -= 2 (drop saved frame)
 15,WTC,1                   . WT := mem[SP+1] = saved return addr
 ,UJ,                       . jump to it
C===========================================
 P/CO:,ENTRY,
C===========================================
C P/CO - create/open a Pascal file.
C
C Setup convention (see FILE.md):
C   M12 = FILE record base
C   M11 = base-type size  (becomes FILE[17], bit-step)
C   M10 = fileBufSize     (sizes the inline buffer)
C   M9  = elSize          (becomes FILE[18], element width)
C   ACC = external file name (FCST literal), or 0 internal file
C
C Re-entered as P/RE1 when only the rewind/reopen logic is
C needed.  *0135B path handles capacity-decrement on subsequent
C calls; the main body computes the buffer layout, packed-mode
C bit-step constants and stashes M11/SP into FILE[3]/[4]/[5] and
C runtime scratch slots [M1+3..5].
 ,NTR,3                     . normalize ACC tag
 1,ATX,3                    . [M1+3] := caller's FCST literal
 12,ATX,32B                 . FILE[26] := ext name
 ,ITA,13                    . ACC := M13 (return addr)
 15,XTS,-3                  . push it; ACC := caller's flag word
 ,UZA,*0135B                . zero => re-open path (P/CO again)
 12,XTA,33B                 . ACC := FILE[27] (capacity)
 1,A-X,10B                  . ACC -= [M1+8] = 1U
 12,ATX,33B                 . FILE[27] := --capacity
 12,XTA,33B                 . re-read for sign test
 ,U1A,*0632B                . underflow -> P/IT trap
C===========================================
 P/RE1:,ENTRY,
C===========================================
C P/RE1 - reposition / reopen helper, also reached internally
C  via the fall-through above.  Decides between the read-side
C  (FILE[4]=0) and write-side (FILE[4]!=0) prep paths.
 12,XTA,27B                 . ACC := FILE[23] (I/O kind bits)
 1,AAX,10B                  . AND with [M1+8] = 1 (stdin/out bit)
 ,UZA,*0045B                . not stdin/out -> skip flush
 12,XTA,11B                 . check FILE[9] (in-progress bits)
 ,UZA,*0045B
 13,VJM,*0571B              . flush partial line (P/WL inner)
 *0045B:12,XTA,4            . ACC := FILE[4] (open-mode)
 ,U1A,*0050B                . non-zero => output path
 13,VJM,*0740B              . input path: open helper
 *0047B:14,VTM,*0632B       . arm M14 to point at P/IT trap
 ,UJ,*0364B                 . jump to advance-buffer helper
 *0050B:13,VJM,*0676B       . output: reset helper
 12,XTA,16B                 . ACC := FILE[14] (packed flag)
 ,UZA,*0047B                . text file -> back to common tail
 12,XTA,11B                 . non-zero bit-counter?
 ,UZA,*0047B
 12,XTA,22B                 . ACC := FILE[18] (elem width)
 ,U1A,*0061B                . non-zero => alternate calc
 12,XTA,21B                 . ACC := FILE[17] (bit step)
 1,AOX,11B                  . tag as integer ([M1+9])
 12,A*X,11B                 . multiply by FILE[9]
 ,YTA,30B                   . extract product fraction
 *0056B:1,X-A,10B           . ACC := 1 - ACC
 12,ATX,13B                 . FILE[11] := computed window upper
 13,VJM,*0642B              . sub-helper
 13,VTM,*0047B              . link back to *0047B
 ,UJ,*0275B                 . jump into PASCTRP tail
 *0061B:12,XTA,16B          . ACC := FILE[14]
 12,ATX,23B                 . FILE[19] := packed flag
 12,XTA,11B                 . ACC := FILE[9]
 1,A-X,10B                  . ACC -= 1
 1,AOX,11B                  . re-tag as integer
 ,UTC,*0754B.=2             . WT := constant 2
 ,XTS,                      . push ACC, ACC := 2
 12,A-X,25B                 . ACC -= FILE[21]
 1,AOX,11B
 13,VJM,P/DI                . call integer divide
 1,AEX,11B                  . untag quotient
 1,ARX,10B                  . ACC += 1U
 ,UJ,*0056B                 . merge with text path
C===========================================================
C *0070B / *0074B / *0124B / *0130B / *0133B / *0135B form
C  the FCST-literal decoder.  P/CO loads the FCST word into
C  [M1+3] and peels off bit fields (3-bit, 7-bit, 13-bit) to
C  populate FILE[3], FILE[4] and the stdin/out flag in
C  FILE[23].  P/BEXF translates a non-zero FCST low-byte into
C  an external file name and returns via *0070B.
C===========================================================
 *0070B:1,ATX,3             . save BEXF result back into [M1+3]
 ,UJ,*0074B
 *0071B:1,XTA,3             . ACC := raw FCST word
 ,UTC,*0755B.=:0077         . WT := mask 0o77
 ,AAX,                      . isolate low 6 bits
 13,VTM,*0070B              . link for BEXF return
 ,U1A,P/BEXF                . if any low bits, look up ext name
 *0074B:1,XTA,3             . reload FCST (or BEXF result)
 ,ASN,64-7                  . normalise the field positions
 ,ASN,64+13
 1,ATX,3                    . [M1+3] := repositioned bits
 14,VTM,*0026B              . M14 := address of bit-1 mask
 14,AAX,                    . AND with that mask
 ,U1A,*0124B                . bit set => stdin/out branch
 1,XTA,3                    . else extract FILE[4] field
 ,ASN,64+21
 12,ATX,4                   . FILE[4] := open-mode bits
 1,XTA,3
 ,ASN,64+3
 ,UTC,*0757B.=77 7777
 ,AAX,                      . mask to FILE[3] width
 12,ATX,3                   . FILE[3] := mode/state byte
 ,UJ,*0147B                 . join common tail
C ---- *0104B/*0105B: default values for FILE[20]/[21]      ----
C  Two more LOG constants, sister table to *0024B..  Used by
C  the stdin/stdout (text-mode) prep below so that the *0513B
C  packer sees a sane shift-amount and loop-count even when no
C  real packed math was performed.
 *0104B:,LOG,70             . 56 - default FILE[20] (shift)
 ,LOG,7 7774                . -4 in 17-bit signed = FILE[21]
C ---- *0106B: stdin/stdout buffer layout (no real buffer) ----
 *0106B:10,J+M,15           . M15 := M15 + M10 (alloc fileBufSize)
 ,ITA,15
 12,ATX,15B                 . FILE[13] := current SP
 12,ATX,                    . FILE[0]  := SP (head=tail)
 15,UTM,6                   . reserve 6 sentinel words
 ,ITA,15
 12,ATX,1                   . FILE[1] := end-of-window sentinel
 14,VTM,*0104B              . M14 -> defaults table
 14,XTA,                    . ACC := mem[*0104B] = 56
 12,ATX,24B                 . FILE[20] := 56 (default shift amt)
 14,XTA,1                   . ACC := mem[*0105B] = -4
 12,ATX,25B                 . FILE[21] := -4 (default loop cnt)
 ,ASN,64-20                 . shift right 20 to derive...
 12,ATX,11B                 . FILE[9]  := signed-shifted -4
 1,XTA,25B                  . ACC := [M1+21] = MSB constant
 12,ATX,26B                 . FILE[22] := MSB seed
 ,XTA,                      . ACC := 0
 12,ATX,3                   . FILE[3]  := 0
 12,ATX,4                   . FILE[4]  := 0 (text/input)
 12,ATX,7                   . FILE[7]  := 0 (FILE[7] WTC slot)
 12,XTA,16B                 . ACC := FILE[14] (packed flag)
 ,UZA,*0215B                . text -> *0215B (joins P/RE2)
 ,XTA,
 12,ATX,11B                 . packed: FILE[9] := 0
 1,XTA,10B                  . ACC := [M1+8] = 1U
 12,ATX,2                   . FILE[2] := 1 (pending)
 ,UJ,*0220B                 . join exit tail
C ---- stdin/stdout flavour (FCST bit 1 set) ----
 *0124B:1,XTA,3
 ,ASN,64+3
 14,AEX,1                   . XOR with *0026B+1 = halfword 2
 ,UZA,*0130B                . pure stdout
 1,AEX,10B                  . else XOR with 1U
 ,UZA,*0133B                . pure stdin
 ,UJ,*0230B                 . else error: "NO BIND EXT FILE"
 *0130B:,XTA,
 10,VTM,16B                 . M10 := offset 14 (FILE[14])
 *0131B:12,ATX,16B          . FILE[14] := 0 (text mode)
 14,XTA,                    . ACC := mem[M14] = bit-1 mask
 12,ATX,27B                 . FILE[23] := mask (stdin/out tag)
 ,UJ,*0147B
 *0133B:14,VTM,*0031B       . M14 -> literal 3 at *0031B
 1,XTA,10B
 10,VTM,25B                 . M10 := 21 (FILE[21])
 ,UJ,*0131B
C ---- *0135B: re-entry from caller flag word = 0 ----
 *0135B:1,XTA,3
 ,UZA,*0141B
 1,XTA,10B
 12,A+X,33B                 . FILE[27] += 1
 12,ATX,33B
 1,AEX,10B                  . sanity-check the result
 ,U1A,*0632B                . mismatch -> trap
 *0141B:,XTA,               . re-init defaults
 12,ATX,2                   . FILE[2]  := 0
 12,ATX,27B                 . FILE[23] := 0
 12,ATX,31B                 . FILE[25] := 0
 1,XTA,10B
 12,ATX,10B                 . FILE[8]  := 1U
 1,XTA,3                    . ACC := [M1+3]
 ,U1A,*0071B                . FCST non-zero -> decode it
 12,ATX,4                   . else FILE[4] := 0
 12,ATX,3                   . FILE[3] := 0
 1,XTA,10B
 12,ATX,33B                 . FILE[27] := 1U
C ---- *0147B: common tail used by both setup branches ----
 *0147B:,ITA,9              . ACC := M9 = elSize
 12,ATX,22B                 . FILE[18] := elSize (bits)
 ,ITA,11                    . ACC := M11 = base-type size
 12,STX,21B                 . FILE[17] := M11 (bit-step)
 1,STX,3                    . [M1+3] := M11 (scratch save)
 1,STX,4                    . [M1+4] := M11 (scratch save)
 ,ITA,15                    . ACC := SP
 12,ATX,14B                 . FILE[12] := SP (buffer start)
 12,ATX,23B                 . FILE[19] := SP (current cursor)
 12,ATX,                    . FILE[0]  := SP
 12,XTA,27B                 . ACC := FILE[23]
 ,UTC,*0026B                . WT := *0026B (bit-1 mask)
 ,AAX,
 ,U1A,*0106B                . stdin/stdout -> minimal layout
 15,UTM,36B                 . else reserve 30 buffer words
 ,ITA,10                    . ACC := M10 = fileBufSize
 ,ASN,64-8                  . convert to words
 1,ATX,5                    . [M1+5] := word-count scratch
 12,A-X,21B                 . ACC -= FILE[17] (bit-step)
 ,U1A,*0226B                . not aligned -> *0226B
 1,XTA,5                    . else compute remainder...
 12,XTS,21B                 . push ACC, load FILE[17]
 14,VJM,*0014B              . call P/MOD inline
 1,X-A,5                    . ACC := [M1+5] - ACC
 15,ATX,                    . push remainder
 *0164B:12,ARX,14B          . ACC += FILE[12] (buf start)
 12,STX,15B                 . FILE[13] := buffer end
 1,X-A,10B                  . ACC := 1U - ACC
 12,ATX,13B                 . FILE[11] := buffer-window cap
 9,VTM,*0024B               . M9 -> literal table at *0024B..
 12,XTA,22B                 . ACC := FILE[18] (elSize bits)
 ,UZA,*0201B                . text -> skip packed prep
 9,X-A,                     . ACC = mem[M9+0] - elSize = 64-eSz
 12,ATX,24B                 . FILE[20] := 64 - elSize (shift amt)
 9,XTA,1                    . ACC := mem[M9+1] = 48 (word bits)
 12,XTS,22B                 . push 48, ACC := elSize
 14,VJM,*0020B              . ACC := 48 / elSize (elems/word)
 1,ATX,5                    . [M1+5] := elems/word
 9,X-A,2                    . ACC := mem[M9+2] - elems = 50 - q
 12,ATX,25B                 . FILE[21] := 50 - (48/elSize)
 9,XTA,1                    . ACC := 48 again
 12,XTS,22B                 . push 48, ACC := elSize
 14,VJM,*0014B              . ACC := 48 mod elSize (remainder)
 9,X-A,                     . ACC := 64 - remainder
 ,ASN,64-41                 . shift right 41 to fit in low bits
 12,ATX,26B                 . FILE[22] := neg shift seed
 1,XTA,5                    . ACC := [M1+5] = elems/word
 12,ARX,15B                 . ACC += FILE[13]
 12,ATX,1                   . FILE[1] := wrap sentinel
 ,UJ,*0202B
 *0201B:12,XTA,15B          . text mode: just copy FILE[13]
 12,ATX,1
 *0202B:,ATI,15             . SP := computed buffer top
 ,XTA,
 12,ATX,7                   . FILE[7] := 0 (clear WTC operand)
 13,VJM,*0726B              . call P/TF helper (output finish)
C===========================================
 P/RE2:,ENTRY,
C===========================================
C P/RE2 - second half of the reset (rewind) sequence.  Splits
C  on FILE[4] to choose between the packed and text branches.
C  Lands at *0210B which programs FILE[2]/FILE[8] for a pending
C  read and forks into the disk-read helper (*0421B = PASINBUF).
 12,XTA,4                   . ACC := FILE[4] (open-mode)
 ,UZA,*0234B                . input file -> *0234B
 ,ASN,64-10                 . shift to expose width nibble
 12,XTS,22B                 . push, ACC := FILE[18]
 ,U1A,*0223B                . non-zero -> packed remainder calc
 12,XTA,21B                 . ACC := FILE[17] (bit step)
 14,VJM,*0020B              . P/DIV inline
 *0210B:12,ATX,11B          . FILE[9]  := result
 12,ATX,12B                 . FILE[10] := shadow copy
 ,UZA,*0230B                . zero -> "NO BIND EXT FILE"
 ,XTA,                      . else clear staged state
 12,ATX,2                   . FILE[2]  := 0 (no pending read)
 12,ATX,16B                 . FILE[14] := 0
 1,XTA,10B
 12,ATX,10B                 . FILE[8]  := 1U (f^ valid)
 13,VTM,*0220B              . link return through *0220B
 ,UJ,*0421B                 . jump to PASINBUF
 *0215B:13,VJM,*0403B       . text-mode fetch + EOF check
 ,UTC,*0753B.=120           . WT := constant 0o120
 ,XTA,
 12,ATX,11B                 . FILE[9] := 0o120
 *0220B:,XTA,
 12,ATX,30B                 . FILE[24] := 0 (clear EOLN)
 1,XTS,4                    . push [M1+4]
 1,XTS,3                    . push [M1+3]
 ,ATI,13                    . M13 := ACC (link)
 13,UJ,                     . return to caller
 *0223B:15,XTA,             . pop divide pre-result
 1,AOX,11B
 1,A*X,5                    . ACC *= [M1+5]
 ,YTA,30B
 ,UJ,*0210B
 *0226B:12,XTA,21B          . join from *0162B (aligned case)
 15,ATX,
 ,UJ,*0164B
 *0230B:10,VTM,*0231B.=6H NO BI
 ,UJ,*0000B
 *0231B:,ISO,18H NO BIND EXT FILE
 *0234B:12,WTC,14B          . input mode: WT := FILE[12]
 ,ATX,                      . zero the buffer slot
 12,XTA,22B
 ,UZA,*0220B                . text -> common exit
 13,VTM,*0220B
 ,UJ,*0351B                 . join P/RACPAK
C===========================================
 PASCTRP:,ENTRY,
C===========================================
C PASCTRP - "PASCAL CONTROL/TRACKS Pre-allocate".  Reserves
C  packed-file disk tracks before the first physical write,
C  using the runtime track table at [M1+37B].
C
C   FILE[3]/FILE[4] hold the file's track-table descriptor.
C   FILE[5]         points at the running track-window cursor.
C   FILE[6]         is the lane mask (0o76000).
C   [M1+37B]        scratches the FCST literal that the
C                   `1,WTC,37B' instruction in the loop reads
C                   to obtain the next track header to clear.
C
C Reports "NO EXTFILE TRAKCS" if the table is empty or
C  "NO LOCFILE TRAKCS" if the local pool is exhausted.
 *0237B:12,XTA,4            . ACC := FILE[4]
 ,U1A,*0241B
 12,XTA,3                   . check FILE[3]
 ,UZA,*0247B
 *0241B:12,XTA,6            . ACC := FILE[6] (lane mask)
 ,UTC,*0760B.=7 6000        . constant 0o76000
 ,AEX,                      . sentinel test
 *0243B:13,UZA,             . if no work, return via M13
 12,XTA,4
 ,ATI,14                    . M14 := FILE[4]
 ,UTC,*0752B.=:001
 ,XTA,
 14,VZM,*0276B              . M14==0 -> *0276B (allocate one)
 12,AOX,3                   . else OR FILE[3] into ACC
 ,UJ,*0305B
 *0247B:12,XTA,4
 ,UZA,*0263B
 12,XTA,5
 1,ARX,10B                  . ACC += 1U  ([M1+8])
 12,ATX,5                   . FILE[5] += 1
 12,A-X,4                   . compare to FILE[4]
 13,U1A,                    . overflow -> return
 :10,VTM,*0254B.=6H NO EX
 ,UJ,*0000B
 *0254B:,ISO,18H NO EXTFILE TRAKCS
 *0257B:10,VTM,*0260B.=6H NO LO
 ,UJ,*0000B
 *0260B:,ISO,18H NO LOCFILE TRAKCS
 *0263B:1,XTA,37B           . ACC := [M1+31] (track-table head)
 ,UZA,*0257B                . empty -> "NO LOCFILE TRAKCS"
 12,XTA,3                   . check FILE[3]
 ,UZA,*0267B
 1,XTA,37B                  . track[0]
 12,WTC,5                   . WT  := FILE[5] (window cursor)
 ,ATX,                      . store next track
 ,UJ,*0270B
 *0267B:1,XTA,37B
 12,ATX,3                   . FILE[3] := track id
 *0270B:1,XTA,37B
 12,ATX,5                   . FILE[5] := track id
 1,WTC,37B                  . WT  := [M1+31]
 ,XTA,                      . pop entry from [M1+31] head
 1,ATX,37B                  . store back next-pointer
 ,XTA,                      . ACC := 0
 12,WTC,5
 ,ATX,                      . zero the freshly-claimed track
 13,UJ,                     . return
 *0275B:12,XTA,4
 ,U1A,*0304B
 *0276B:12,WTC,5            . WT := FILE[5]
 ,AOX,1                     . ACC |= mem[WT+1]
 *0277B:15,ATX,1            . save scratch on stack
 12,WTC,7
 ,XTA,1
 ,ASN,64-20                 . shift right 20 to repack
 15,AOX,1
 15,ATX,1
 15,*70,1                   . SP-relative store w/auto-pop
 13,UJ,
 *0304B:12,XTA,3
 *0305B:12,ARX,5            . ACC += FILE[5]
 ,UJ,*0277B
 *0306B:10,VTM,*0307B.=6H GET(F
 ,UJ,*0000B
 *0307B:,ISO,18H GET(F) EOF=TRUE
C===========================================
 P/GF:,ENTRY,
C===========================================
C P/GF - get next element of FILE @ M12 into f^ at [M1+8].
C
C Errors out "GET(F) EOF=TRUE" if FILE[2] (pending flag) is
C clear; otherwise either advances the in-window bit cursor
C (packed mode) or refills from disk via PASINBUF/*0403B.
 ,NTR,3
 12,XTA,2                   . ACC := FILE[2] (pending flag)
 ,U1A,*0306B                . zero -> EOF abort
 12,XTA,10B                 . ACC := FILE[8] (f^)
 12,AEX,11B                 . XOR with FILE[9]
 ,UZA,*0330B                . stdin path / EOLN handling
 12,XTA,10B
 1,ARX,10B                  . ACC |= 1U (caller-bit mask)
 12,ATX,10B                 . FILE[8] := f^ | 1U
 *0317B:12,XTA,22B          . ACC := FILE[18] (elem width)
 ,UZA,*0345B                . text mode -> *0345B
 12,XTA,                    . packed: ACC := FILE[0] (bit ptr)
 12,ARX,21B                 . += FILE[17] (bit step)
 12,ATX,
 12,AEX,1                   . compare to FILE[1] (sentinel)
 13,U1A,                    . not wrapped -> return
 12,XTA,23B                 . ACC := FILE[19] (window cursor)
 1,ARX,10B
 12,ATX,23B                 . FILE[19] += 1
 12,AEX,15B                 . compare to FILE[13]
 ,U1A,*0351B                . overflow -> unpack via P/RACPAK
 12,XTA,2
 ,UZA,*0421B                . if pending cleared -> PASINBUF
 *0326B:14,VJM,*0722B       . else flush via *0722B (P/TF helper)
 13,UJ,                     . return via M13
 *0330B:12,XTA,27B          . ACC := FILE[23] (I/O kind bits)
 ,UTC,*0026B                . WT  := bit-1 mask
 ,AAX,
 ,UZA,*0336B                . not stdin/out -> *0336B
 12,XTA,30B                 . ACC := FILE[24] (EOLN flag)
 1,AEX,10B                  . XOR with 1U
 12,ATX,30B                 . toggle EOLN
 ,UZA,*0403B                . trigger disk read on transition
 14,VTM,SPACE*              . else f^ := ' '
 ,ITA,14
 12,ATX,                    . FILE[0] := ' ' addr
 13,UJ,
 *0336B:1,XTA,10B           . normal stdin path
 12,ATX,2                   . FILE[2] := 1U
 12,XTA,16B                 . ACC := FILE[14] (packed flag)
 ,U1A,*0317B                . packed -> join *0317B
 12,XTA,20B                 . text: ACC := FILE[16] (saved [6])
 12,ATX,6                   . FILE[6] := descriptor
 12,XTA,5                   . ACC := FILE[5] (shift)
 12,AEX,17B                 . compare with FILE[15] (wrap)
 ,UZA,*0317B
 14,VJM,*0364B              . wrap via advance-buf helper
 12,XTA,17B                 . FILE[5] := FILE[15]
 12,ATX,5
 ,UJ,*0317B
 *0345B:12,XTA,             . text-mode bit advance
 12,ARX,21B                 . ACC += FILE[17]
 12,ATX,                    . FILE[0] := ACC
 12,AEX,15B                 . compare with FILE[13]
 13,U1A,                    . no overflow -> return
 12,XTA,2
 ,UZA,*0421B
 ,UJ,*0326B
C===========================================
 P/RACPAK:,ENTRY,
C===========================================
C P/RACPAK - "Read Accumulator Packed".  Unpacks one packed
C  element from the disk-loaded buffer.  FILE[20] supplies the
C  ASN shift amount (= 64 - elSize), FILE[21] the negative VLM
C  loop count, FILE[22] the second-stage ASX shift, and FILE[25]
C  the post-OR pattern (sign/tag bits).  All four are plain
C  numbers - no instruction patching takes place.
 *0351B:12,XTA,1            . ACC := FILE[1] (window end)
 ,ATI,9
 12,XTA,24B                 . ACC := FILE[20] (ASN shift amount)
 ,ATI,10                    . M10 := shift amount
 12,WTC,25B                 . WT  := FILE[21] (loop count base)
 11,VTM,-1
 12,WTC,23B                 . WT  := FILE[19] (cursor)
 ,XTA,
 15,ATX,1                   . push first word read
 *0356B:15,XTA,1            . loop: re-load it
 10,ASN,                    . shift ACC by M10 (= FILE[20])
 15,ATX,1
 ,YTA,                      . extract bits shifted out
 12,AEX,31B                 . XOR with FILE[25] (sign/tag mask)
 11,UTC,-1
 9,ATX,                     . store at FILE[1]+M9 slot
 11,VLM,*0356B              . VLM: ++M11; loop while M11 != 0
 12,XTA,15B                 . ACC := FILE[13]
 12,ATX,                    . FILE[0] := FILE[13] (reset cursor)
 13,UJ,
C ---- *0364B: advance buffer iterator (M1+29/+30 cache) ----
C  Called from P/GF text path and PASGIVEP to step the
C  FILE[7] WTC operand by one element.  Uses [M1+29] as the
C  previous-FILE[7] snapshot and [M1+36B] as a copy of it.
 *0364B:12,XTA,7            . ACC := FILE[7]
 14,UZA,                    . FILE[7]=0 -> return via M14
 1,AEX,35B                  . compare to [M1+29] cache
 ,U1A,*0370B                . changed -> slow path
 12,WTC,7                   . WT := FILE[7]
 ,XTA,                      . ACC := mem[WT]
 1,ATX,35B                  . refresh [M1+29] := new value
 ,UJ,*0377B
 *0370B:1,XTA,35B           . pull cached value
 15,ATX,1                   . save on stack
 *0371B:15,WTC,1            . loop scanning ahead
 ,XTA,
 12,AEX,7
 ,UZA,*0375B
 15,WTC,1
 ,XTA,
 15,ATX,1
 ,UJ,*0371B
 *0375B:12,WTC,7
 ,XTA,
 15,WTC,1
 ,ATX,                      . write new buffer slot
 *0377B:1,XTA,36B           . ACC := [M1+30] (working ptr)
 12,WTC,7
 ,ATX,
 12,XTA,7
 1,ATX,36B                  . refresh [M1+30] := FILE[7]
 ,XTA,
 12,ATX,7                   . FILE[7] := 0 (consumed)
 14,UJ,                     . return via M14
C ---- *0403B: physical-read helper (called from P/GF/P/RE2)
C  Invokes PASEOF (MONCARD test) then READ* if data is real.
 *0403B:,ITA,13
 1,XTS,10B                  . push link, ACC := [M1+8]
 12,ATX,10B                 . stash f^ in FILE[8]
 13,VJM,PASEOF              . check end-of-input card
 ,UZA,*0415B                . EOF -> common tail
 ,ITA,8
 ,ITS,12                    . push M8, M12
 15,ATX,
 12,XTS,14B                 . push FILE[12] (buf start)
 13,VJM,READ*               . disk-zone read
 ,NTR,3
 15,XTA,
 ,STI,12                    . restore M12
 ,ATI,8                     . restore M8
 12,WTC,14B
 ,XTA,
 ,UTC,PASEOFCD              . compare to EOF sentinel
 ,AEX,
 ,U1A,*0417B
 *0415B:1,XTA,10B           . on EOF: reset f^
 12,ATX,2                   . FILE[2] := f^
 12,ATX,30B                 . FILE[24]:= f^
 *0417B:14,VJM,*0722B       . flush via *0722B
 13,VTM,*0632B
 ,UJ,*0351B                 . unpack via P/RACPAK
C===========================================
 PASINBUF:,ENTRY,
C===========================================
C PASINBUF - allocate/refill the input buffer.  Called from
C  P/RE2 (*0210B -> *0421B) once the file is opened.
C  Pushes M13 then snapshots FILE[5]/FILE[6] into FILE[15]/
C  FILE[16] and FILE[13] into FILE[19] so that PASGIVEP can
C  restore them later, then runs a VLM-driven copy loop
C  (*0433B/*0434B) to scoop disk records into the inline
C  buffer.  If we run out we tail-call PASCTRP at *0243B to
C  claim more tracks.
 *0421B:,ITA,13
 12,XTS,6                   . push M13, ACC := FILE[6]
 ,ATI,10                    . M10 := FILE[6]
 12,XTA,7
 ,U1A,*0425B                . FILE[7]!=0 -> already initialised
 13,VJM,*0456B              . call PASGIVEP to release old buf
 10,VZM,*0425B
 13,VJM,*0243B              . allocate fresh tracks
 *0425B:12,WTC,7
 ,XTA,1
 ,ATI,9
 9,UTM,1777B                . M9 := 1023 (zone size - 1)
 12,XTA,5
 12,ATX,17B                 . FILE[15] := FILE[5]
 12,XTA,6
 12,ATX,20B                 . FILE[16] := FILE[6]
 12,XTA,15B
 ,ATI,13                    . M13 := FILE[13]
 12,XTA,13B
 ,ATI,14                    . M14 := FILE[11]
 *0433B:10,VLM,*0434B       . VLM loop: copy disk -> buffer
 ,UJ,*0445B
 *0434B:9,UTC,
 10,XTA,                    . ACC := next disk word
 13,UTC,-1
 14,ATX,                    . store into buffer
 14,VLM,*0433B
 ,ITA,10
 *0437B:12,ATX,6            . FILE[6] := M10 (descriptor)
 12,XTA,14B
 12,ATX,23B                 . FILE[19] := FILE[12]
 12,ATX,                    . FILE[0]  := FILE[12]
 ,UJ,*0667B
 *0442B:12,XTA,5
 1,ARX,10B                  . ACC += 1U
 15,ATX,1
 12,AEX,4
 ,UZA,*0437B
 ,UJ,*0450B
 *0445B:12,XTA,4
 ,U1A,*0442B
 12,WTC,5
 ,XTA,
 15,ATX,1
 ,UZA,*0437B
 *0450B:15,XTA,1
 12,ATX,5
 ,ITA,13
 ,ITS,14                    . push M14, ACC := M13
 1,XTS,10B
 13,VJM,*0243B              . request more tracks
 15,XTA,
 ,STI,14
 ,ATI,13
 10,VTM,76001B
 ,UJ,*0434B                 . resume copy loop
C===========================================
 PASGIVEP:,ENTRY,
C===========================================
C PASGIVEP - "PASCAL GIVE Partial".  Flushes any partially
C  filled packed-output word back to its rightful place in
C  the buffer using [M1+29B] (saved cursor) and [M1+30B]
C  (working pointer) to walk the touched range.  Final pass
C  at *0471B fires off the disk write through *0275B.
 *0456B:12,XTA,7            . ACC := FILE[7]
 13,U1A,                    . FILE[7]=0 -> nothing to do
 ,ITA,13                    . ACC := M13 (link)
 1,XTS,36B                  . push link, ACC := [M1+30B]
 ,U1A,*0501B                . non-zero -> resume mid-flush
 1,XTA,35B                  . else seed [M1+30B] from cache
 1,ATX,36B
 *0462B:1,WTC,36B           . loop: WT := [M1+30B]
 ,XTA,
 ,UZA,*0467B                . hit zero -> done
 1,XTA,36B
 15,ATX,1                   . save current ptr
 1,WTC,36B
 ,XTA,
 1,ATX,36B                  . advance [M1+30B]
 ,UJ,*0462B
 *0467B:1,XTA,36B
 1,AEX,35B                  . compare with cache
 ,U1A,*0511B                . diverged -> *0511B
 1,ATX,35B
 *0471B:,ITA,12             . ACC := M12
 15,ATX,
 1,WTC,36B
 ,WTC,2
 12,VTM,
 12,XTA,2
 ,UZA,*0477B
 12,XTA,11B
 12,AEX,12B
 ,UZA,*0477B
 13,VJM,*0275B              . disk write via track helper
 *0477B:,XTA,
 12,ATX,7                   . FILE[7] := 0
 15,XTA,
 ,ATI,12                    . restore M12
 *0501B:1,XTA,36B
 15,ATX,1
 1,WTC,36B
 ,XTA,
 1,ATX,36B
 1,XTA,35B
 15,WTC,1
 ,ATX,
 15,XTA,1
 1,ATX,35B
 12,ATX,7
 ,ITA,12
 15,WTC,1
 ,ATX,2
 15,WTC,
 ,UJ,                       . indirect return
 *0511B:,XTA,
 15,WTC,1
 ,ATX,
 ,UJ,*0471B
C===========================================================
C *0513B - "Pack ACC into the current buffer slot".  Helper
C  used by P/PF and P/WL.  Loads M10/M11 from FILE[20]/[21]
C  (shift amount and -loop count, both plain numbers), walks
C  M9 down by the element width and OR-folds the new bits
C  into the word at FILE[19].
C===========================================================
 *0513B:12,XTA,15B          . ACC := FILE[13]
 12,ATX,                    . FILE[0] := FILE[13] (reset cursor)
 12,XTA,24B                 . ACC := FILE[20] (ASN shift amount)
 ,ATI,10                    . M10 := shift amount
 12,XTA,25B                 . ACC := FILE[21] (VLM loop count)
 ,ATI,11                    . M11 := loop count (negative)
 12,XTA,1                   . ACC := FILE[1]
 ,ATI,9
 12,XTA,31B                 . ACC := FILE[25] (post-OR mask)
 ,UZA,*0525B
 11,UTM,-1
 *0521B:9,UTC,-1            . slow loop: pack one word at a time
 11,XTA,
 1,AAX,14B                  . AND with [M1+12]=positive-mant mask
 9,UTC,-1
 11,ATX,
 11,VLM,*0521B
 12,XTA,25B
 ,ATI,11
 *0525B:,XTA,
 *0526B:9,UTC,-2            . tight loop: shift+OR
 11,AEX,
 10,ASN,                    . shift ACC by M10 = FILE[20]
 11,VLM,*0526B
 9,AEX,-1
 12,ASX,26B                 . ASX ACC by FILE[22] (-shift)
 12,WTC,23B                 . WT := FILE[19] (cursor)
 ,ATX,                      . merge result into buffer
 14,UJ,                     . return via M14
 *0533B:10,VTM,*0534B.=6H PUT(F
 ,UJ,*0000B
 *0534B:,ISO,18H PUT(F) EOF=FALSE
C===========================================
 P/PF:,ENTRY,
C===========================================
C P/PF - put one element from f^ to FILE @ M12.
C  Mirror of P/GF: bumps FILE[9] (counter), updates the bit
C  cursor at FILE[0], optionally packs via *0513B and trips
C  the disk-write helpers when the window fills up.
C  Errors "PUT(F) EOF=FALSE" if FILE[2] is already clear.
 *0537B:,NTR,3
 12,XTA,2                   . ACC := FILE[2]
 ,UZA,*0533B                . zero -> EOF abort
 12,XTA,11B
 1,ARX,10B                  . FILE[9] += 1U
 12,ATX,11B
 12,XTA,
 12,ARX,21B                 . FILE[0] += FILE[17]
 12,ATX,
 12,AEX,1                   . compare with FILE[1]
 13,U1A,                    . not wrapped -> return
 12,XTA,22B                 . ACC := FILE[18] (elem width)
 ,UZA,*0551B                . text mode -> *0551B
 14,VJM,*0513B              . packed: pack into buffer
 12,XTA,23B
 12,A+X,21B                 . FILE[19] += FILE[17]
 12,ATX,23B
 12,AEX,15B                 . compare with FILE[13]
 13,U1A,
 *0551B:12,XTA,27B          . ACC := FILE[23]
 ,UTC,*0026B
 ,AAX,
 ,U1A,*0600B                . stdin/out -> *0600B (write line)
 ,ITA,13
 *0554B:12,XTS,6            . push link, ACC := FILE[6]
 ,ATI,10
 ,XTA,
 12,ATX,16B                 . FILE[14] := 0
 12,XTA,22B                 . ACC := FILE[18]
 ,U1A,*0560B
 12,XTA,
 12,ATX,23B                 . FILE[19] := FILE[0]
 *0560B:12,XTA,7
 ,U1A,*0563B
 13,VJM,*0456B              . flush partial via PASGIVEP
 13,VJM,*0237B              . request more tracks (PASCTRP)
 *0563B:12,WTC,7
 ,XTA,1
 ,ATI,9
 9,UTM,1777B                . M9 := 1023
 12,XTA,23B
 ,ATI,13
 12,XTA,13B
 ,ATI,14
 *0567B:10,VLM,*0626B       . VLM disk-write loop
 ,UJ,*0633B                 . join end-of-write tail
C===========================================
 P/WOLN:,ENTRY,
C===========================================
C P/WOLN - "Write OUTPUT Line".  Wires M12 to the *OUTPUT*
C  FILE record, then falls through into P/WL.
 12,VTM,*OUTPUT*
C===========================================
 P/WL:,ENTRY,
C===========================================
C P/WL - terminate the current line on FILE @ M12.  Pushes
C  the LF char (PASEOLSY) into the buffer, then funnels into
C  *0537B (= P/PF) to ship the data out.  Handles the
C  page-eject case at *0600B/*0617B for the line-printer.
 *0571B:12,XTA,27B          . ACC := FILE[23]
 1,AAX,10B
 ,U1A,*0600B
 ,UTC,PASEOLSY              . ACC := LF char
 ,XTA,
 12,WTC,                    . WT := FILE[0]
 ,ATX,                      . buffer[FILE[0]] := LF
 ,UJ,*0537B                 . fall through to P/PF
 *0575B:12,WTC,14B          . inner helper for P/WOLN trailer
 10,VTM,
 12,WTC,23B
 11,VTM,
 14,UJ,
 *0600B:,ITA,13             . line-printer terminator path
 ,ITS,2                     . save M2
 12,XTS,
 ,ATI,2
 12,AEX,15B
 ,U1A,*0617B
 14,VJM,*0575B
 ,UTC,PASENDS*              . PASENDS* = page-feed control
 ,XTA,
 ,ASN,64-40
 11,ATX,
 *0606B:13,VJM,P/PRINT      . issue PRINT8 to flush record
 12,XTA,15B
 12,AEX,23B
 ,UZA,*0613B
 13,VJM,*0726B              . call P/TF helper
 *0611B:15,XTA,
 ,STI,2
 ,ATI,13                    . restore M13
 13,UJ,                     . return
 *0613B:13,VJM,*0726B
 ,UTC,SPACE*
 ,XTA,
 12,WTC,
 ,ATX,                      . buffer[0] := space
 13,VTM,*0611B
 ,UJ,P/PF                   . issue final PUT
 *0617B:,UTC,PASENDS*       . blank-page path
 *0620B:,XTA,
 2,ATX,
 2,UTM,1
 ,ITA,2
 12,AEX,1
 ,U1A,*0620B
 14,VJM,*0513B              . pack remainder
 14,VJM,*0575B              . header words
 ,UJ,*0606B                 . then print
 *0626B:13,UTC,-1           . disk-write inner VLM body
 14,XTA,
 9,UTC,
 10,ATX,
 14,VLM,*0567B
 ,ITA,10
 12,ATX,6                   . FILE[6] := M10
 14,VJM,*0722B              . trailing close-buffer helper
C ---- *0632B: trap-style return, used everywhere as "P/IT" ----
 *0632B:15,WTC,             . WT := mem[SP+0] (saved link)
 ,UJ,                       . indirect jump to it
C ---- *0633B: tail of *0567B disk-write loop ----
 *0633B:,ITA,14
 ,ITS,13
 ,ITS,12                    . save M12/M13/M14 on stack
 15,ATX,
 13,VJM,*0275B              . request more output tracks
 13,VJM,*0247B              . and validate the track-table
 15,XTA,                    . restore stack frame
 ,STI,12
 ,STI,13
 ,ATI,14
 10,VTM,76001B              . VLM seed (one zone = 1024 words)
 ,UJ,*0626B                 . resume disk-write loop
C ---- *0642B: thin shim used by P/CO and P/GF text path ----
 *0642B:,ITA,13
 ,UJ,*0554B
C===========================================
 P/RF:,ENTRY,
C===========================================
C P/RF - reset a Pascal file for reading.  Tears down any in-
C  flight write state, sets FILE[12]/FILE[19]/FILE[0] to the
C  start of the buffer and turns on the "no element pending"
C  flag so the first GET will trigger PASINBUF/*0421B.
 ,NTR,3
 ,ITA,13                    . save link
 15,ATX,
 13,VJM,*0676B              . flush any pending write
 12,XTA,11B                 . ACC := FILE[9]
 12,ATX,12B                 . FILE[10] := FILE[9]
 12,XTA,14B                 . ACC := FILE[12]
 12,ATX,23B                 . FILE[19] := FILE[12]
 12,ATX,                    . FILE[0]  := FILE[12]
 12,XTA,22B                 . packed? -> rewind window too
 ,UZA,*0652B
 12,XTA,15B
 12,ATX,                    . FILE[0]  := FILE[13]
 *0652B:,UTC,*0760B.=7 6000 . WT := 0o76000 (lane mask)
 ,XTA,
 12,ATX,6                   . FILE[6]  := lane mask
 1,XTA,10B
 12,ATX,10B                 . FILE[8]  := 1U (mark f^ valid)
 12,XTA,4                   . ACC := FILE[4]
 ,U1A,*0657B                . non-zero -> output reset
 12,XTA,3                   . else FILE[3] -> FILE[5]
 12,ATX,5
 ,UJ,*0660B
 *0657B:,XTA,
 12,ATX,5                   . FILE[5] := 0
 *0660B:14,VJM,*0364B       . advance-buf to seed FILE[7]
 12,XTA,11B
 ,U1A,*0664B
 1,XTA,10B
 12,ATX,2                   . FILE[2] := 1U
 ,UJ,*0632B
 *0664B:,XTA,
 12,ATX,2
 12,XTA,16B                 . ACC := FILE[14]
 ,U1A,*0667B
 13,VTM,*0632B
 ,UJ,*0421B                 . refill via PASINBUF
 *0667B:12,XTA,22B
 ,UZA,*0632B
 13,VTM,*0632B
 ,UJ,*0351B                 . unpack via P/RACPAK
 *0671B:12,XTA,7
 ,UZA,*0632B
 12,XTA,6
 ,UTC,*0760B.=7 6000        . check FILE[6] vs lane mask
 ,AEX,
 ,UZA,*0632B
 13,VJM,*0275B
 ,UJ,*0632B
C ---- *0676B: pre-reset sanity check (called from P/RF/P/TF)
 *0676B:12,XTA,2            . ACC := FILE[2]
 13,UZA,                    . zero -> return via M13
 12,XTA,11B
 12,AEX,12B                 . FILE[9] != FILE[10] => buffered
 13,UZA,
 ,ITA,13
 12,XTS,22B                 . push link, ACC := FILE[18]
 ,UZA,*0706B                . text -> *0706B
 12,XTA,
 12,AEX,15B                 . compare FILE[0] with FILE[13]
 ,UZA,*0710B
 14,VJM,*0513B              . packed: pack out final bits
 12,XTA,23B
 1,ARX,10B
 12,ATX,23B
 ,UJ,*0710B
 *0706B:12,XTA,
 12,ATX,23B                 . FILE[19] := FILE[0]
 12,AEX,14B                 . compare to FILE[12]
 ,UZA,*0671B
 *0710B:12,XTA,16B
 ,U1A,*0720B
 12,XTA,14B
 12,AEX,23B
 ,UZA,*0632B
 12,XTA,13B
 1,XTS,10B
 ,NTR,3
 12,A-X,23B
 12,A+X,14B
 12,ATX,13B                 . FILE[11] := FILE[12]+FILE[13]-...
 13,VJM,*0642B              . disk-write helper
 15,XTA,
 12,ATX,13B
 13,VTM,*0632B
 ,UJ,*0275B                 . then claim a track
 *0720B:12,XTA,23B
 12,ATX,16B                 . FILE[14] := FILE[19]
 15,WTC,
 ,UJ,
C ---- *0722B: thin "close current buffer window" helper ----
 *0722B:12,XTA,14B
 12,ATX,23B                 . FILE[19] := FILE[12]
 12,ATX,                    . FILE[0]  := FILE[12]
 12,XTA,22B
 14,UZA,                    . text -> return via M14
 12,XTA,15B                 . else also reset FILE[1] window
 12,ATX,
 14,UJ,
C===========================================
 P/TF:,ENTRY,
C===========================================
C P/TF - truncate file (= rewrite).  Empties the buffer and
C  re-initialises the packed-mode state so a fresh write run
C  can begin.  The *0742B tail walks the [M1+37B] track-table,
C  reading each entry via `1,WTC,37B' (no patching - WTC just
C  uses [M1+37B] as the next-instruction's address operand).
 *0726B:14,VJM,*0722B       . close any open window first
 ,XTA,
 12,ATX,11B                 . FILE[9]  := 0
 12,ATX,12B                 . FILE[10] := 0
 ,UTC,*0760B.=7 6000        . lane mask again
 ,XTA,
 12,ATX,6                   . FILE[6]  := lane mask
 14,VJM,*0364B
 1,XTA,10B
 12,ATX,2                   . FILE[2]  := 1U (pending write)
 12,ATX,16B                 . FILE[14] := 1U
 12,XTA,4
 ,U1A,*0736B
 ,UJ,*0740B
 *0736B:,XTA,               . input file: just clear FILE[5]
 12,ATX,5
 13,UJ,
 *0740B:12,XTA,3            . output: load FILE[3] (track id)
 13,UZA,                    . zero -> just return
 12,ATX,5                   . FILE[5] := FILE[3]
 *0742B:12,WTC,5            . loop: WT := FILE[5]
 ,XTA,
 ,U1A,*0750B                . non-zero -> *0750B
 1,XTA,37B                  . [M1+31] := track id
 12,WTC,5
 ,ATX,                      . store zero into the track header
 12,XTA,3
 1,ATX,37B                  . [M1+31] := FILE[3]
 ,XTA,
 12,ATX,3                   . FILE[3] := 0
 12,ATX,5                   . FILE[5] := 0
 13,UJ,                     . return
 *0750B:12,ATX,5            . FILE[5] := non-zero scan word
 ,UJ,*0742B
C===========================================================
C Literal pool used by the routines above
C===========================================================
 *0751B:,LOG,60             . '0' character (used by PASZERO*)
 *0752B:,OCT,001            . bit-0 mask (PASCTRP track test)
 *0753B:,LOG,120             . 0o120 placeholder for FILE[9]
 *0754B:,LOG,2              . literal 2 (P/CO packed divide)
 *0755B:,OCT,0077           . mask 0o77 (FCST low-byte select)
 *0756B:,INT,12             . field width = 12 (abort handler)
 *0757B:,LOG,77 7777        . mask 0o777777 (FILE[3] width)
 *0760B:,LOG,7 6000         . lane mask 0o076000 (buf state)
 ,END,
