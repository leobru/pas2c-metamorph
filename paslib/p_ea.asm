 P/EA:,NAME,DTRAN  /01.06.84/
C===========================================================
C P/EA - "Element Access" for FORTRAN routine arguments.
C
C The compiler's `genEntry` (base.pas / work.p2c) emits a call
C to this helper when a **formal procedural parameter** is passed
C as an actual to a **FORTRAN** callee.  Helper table index 19
C (`getHelperProc(19)` / `"P/EA    "` in `helperNames`).
C
C Emission site (see doc/genEntry.md, section 3a):
C   if (actualOp = PCALL) or (actualOp = FCALL) then
C       if (passedRoutine^.list <> NIL) then begin   (* formal *)
C           addToInsnList(passedRoutine^.offset + KXTA +
C                         passedRoutine^.value);
C           if (21 in calleeFlags) then              (* FORTRAN *)
C               addToInsnList(getHelperProc(19));     (* P/EA *)
C
C That is the only path to P/EA.  A concrete nested/global
C procedure (`list = NIL`) never calls P/EA; `genEntry` instead
C builds or loads a thunk via `formThunk` and finishes with
C `KVTM,I14,addr` plus either `KITA+14` (FORTRAN routine with
C bit 21) or `P/PB` (Pascal callee).  P/EA is the matching
C adapter for a **formal** procedural value already stored in
C the caller's frame.
C
C Caller convention on entry:
C   M13  = link from `,VJM,P/EA`
C   ACC  = closure pointer loaded by the preceding `KXTA` from
C          the formal's activation-record slot.  For a ROUTINEID
C          lvalue the compiler always uses displacement 3 inside
C          that block (see `formOperator` / STKLVAL on ROUTINEID:
C          `disp := 3`).  Word [3] of the closure holds the entry
C          address P/EA must hand to the FORTRAN convention.
C
C Success path: verify the closure is a plain FORTRAN-compatible
C descriptor (no Pascal thunk metadata in the high bits), fetch
C `closure[3]`, retag it as an integer address, return in ACC via
C `13,UJ`.
C
C Failure path: if the high descriptor bits are non-zero the
C formal still carries a Pascal-style closure that cannot be
C passed to a FORTRAN subprogram; print
C     NOT POSSIBLE PASCAL PROC TO FORTRAN SUBR
C on OUTPUT and HALT through P/HT.
C
C Imports: P/7A, P/WL, P/HT, *OUTPUT*.
C===========================================================
 P/HT:,SUBP,                  . terminate program (helper #35)
 *OUTPUT*:,LC,30              . 30-word FILE record of OUTPUT
 P/7A:,SUBP,                  . write 6-bit alfa: M11 addr, M10 count
 P/WL:,SUBP,                  . write newline
C ---- Main entry (FORTRAN formal proc-actual adapter) ----------
 ,ATI,14                      . M14 := ACC (save closure pointer)
 ,ASN,64+15                   . ACC >>= 15 (isolate descriptor bits
                              .   above the low 15; must be zero for
                              .   a FORTRAN-passable formal)
 ,U1A,*0004B                  . non-zero -> Pascal closure -> error
 14,XTA,3                     . ACC := closure[3] (entry address;
                              .   same offset genEntry uses for ROUTINEID)
 ,UTC,*0017B.=7 7777          . load integer-tag mask ([M1+16])
 ,AAX,                        . retag ACC as a plain integer address
 13,UJ,                       . indirect return; tagged entry in ACC
C ---- Error: Pascal formal cannot satisfy FORTRAN actual ------
 *0004B:11,VTM,*0010B.=6H NOT P
                              . M11 := start of ISO error text
 10,VTM,51B                   . M10 := 24 octal = char count for P/7A
 12,VTM,*OUTPUT*              . M12 := OUTPUT file block
 13,VJM,P/7A                  . write the message fragment
 13,VJM,P/WL                  . terminate the OUTPUT line
 ,UJ,P/HT                     . HALT
 *0010B:,ISO,24H NOT POSSIBLE PASCAL PRO
 ,ISO,18HC TO FORTRAN SUBR
 *0017B:,LOG,7 7777            . integer-address tag constant
 ,END,
