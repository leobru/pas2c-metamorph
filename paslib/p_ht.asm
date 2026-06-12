 P/HT:,NAME,DTRAN  /01.06.84/
C===========================================================
C P/HT - runtime implementation of Pascal's `halt' procedure.
C  (HT = HaLT.  Nothing to do with horizontal tabs.)
C
C  The compiler turns `halt' into a plain `,VJM,P/HT' call
C  (see work.p2c, helper #35 / system proc #6).  P/HT then:
C
C    1.  Invokes the user's post-mortem-dump entry if one was
C        installed at PASPMDAD (the same convention used by the
C        abort handler in P/SYS).
C    2.  Calls P/RC to walk back up the Pascal activation chain
C        whose head is held in M7, running any cleanup linked
C        from each frame's state word at mem[M7+1].
C    3.  Patches the address field of the outermost frame's
C        state word so that the eventual frame-return jumps to
C        STOP* (the runtime's final exit).  This is NOT self-
C        modifying code - mem[M7+1] is a *data* slot in the
C        activation frame, never an instruction in program text.
C    4.  Branches to P/E ("end - clean") or P/EF ("end - forced")
C        depending on whether the patched word came out zero.
C
C  Caller convention:  M13 = link from VJM,  M1 = constant base
C  (P/RC uses the saved M1 to recognise the outermost frame),
C  M7 = top of activation chain (the runtime keeps this current).
C===========================================================
 STOP*:,SUBP,                . runtime final exit (MS Dubna OS)
 P/E:,SUBP,                  . normal Pascal end-of-program
 P/RC:,SUBP,                 . activation-chain unwinder
 P/EF:,SUBP,                 . forced end (run patched cleanup)
 PASPMDAD:,LC,1              . user post-mortem-dump entry, or 0
 14,VTM,PASPMDAD             . M14 := &PASPMDAD
 14,XTA,                     . ACC := PASPMDAD (handler addr)
 ,UZA,*0003B                 . none installed -> skip PMD
 13,VTM,*0003B               . arrange PMD to return to *0003B
 14,WTC,                     . WT  := mem[M14] = PMD handler addr
 ,UJ,                        . jump into the PMD handler
 *0003B:1,MTJ,13             . M13 := M1 (chain-end sentinel for
C                              P/RC: it stops when M7 == saved M13)
 14,VJM,P/RC                 . unwind the activation chain
 7,XTA,1                     . ACC := mem[M7+1] = outer frame's
C                              state/return-link word
 ,UTC,*0011B.=77 7770 0000   . WT  := high-bits mask
 ,AAX,                       . ACC &= mask (clear address field,
C                              keep cleanup/opcode bits)
 14,VTM,STOP*                . M14 := STOP* (runtime exit addr)
 ,ITS,14                     . push STOP* onto the BESM-6 stack
 15,AOX,-1                   . ACC |= mem[SP-1] = STOP*
C                              (install STOP* as the new addr part)
 7,STX,1                     . mem[M7+1] := patched return-link
 ,UZA,P/E                    . if patched word came out zero -> P/E
 ,UJ,P/EF                    . else hand off to P/EF
 *0011B:,LOG,77 7770 0000    . high-bits mask (clears addr field)
 ,END,
