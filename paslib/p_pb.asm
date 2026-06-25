 P/PB:,NAME,DTRAN  /01.06.84/
C===========================================================
C P/PB, P/BP, P/B6, P/B7 - Pascal procedural closure helpers.
C
C All four entries live in this module.  `genEntry` (base.pas /
C work.p2c) is the compiler site that emits calls to P/PB, P/BP,
C and P/B6; P/B7 is reached indirectly via the `macro+18` peephole
C expansion used on **indirect** routine calls (call through a
C formal procedural parameter).
C
C Helper table indices (`helperNames`):
C   62  P/BP  - validate a closure and dispatch a nested call
C   63  P/B6  - tail trampoline at the end of a synthesized thunk
C   64  P/PB  - tag a thunk address as a Pascal proc-actual word
C   65  P/B7  - untag a closure word during an indirect call
C
C See doc/genEntry.md sections 3a.2 .. 3a.5 for the long-form
C compiler-side narrative.
C===========================================================
 P/RSR:,SUBP,                  . static-link resolver (see p_rsr.asm)
 P/WOLN:,SUBP,                 . write *OUTPUT* line + newline
 P/PRINT:,SUBP,                . printf-like core
 P/7A:,SUBP,                   . write 6-bit alfa: M11 addr, M10 count
 P/WI:,SUBP,                   . write integer
 P/WO:,SUBP,                   . write octal word
 P/HT:,SUBP,                   . terminate program (helper #35)
C===========================================================
C P/PB - Pascal "push/block" closure tagger (helper #64).
C
C `genEntry` emits this after `KVTM,I14,thunk_addr` when a
C **concrete** nested/global routine (`list = NIL`) is passed as
C a procedural actual to a **Pascal** callee (callee lacks flag 21).
C FORTRAN callees use `KITA+14` instead; formal proc-actuals use
C `P/EA` (see p_ea.asm).
C
C   addToInsnList(KVTM+I14 + passedRoutine^.value);
C   if 21 in passedRoutine^.flags then
C       addToInsnList(KITA+14)
C   else
C       addToInsnList(getHelperProc(64));   (* P/PB *)
C
C On entry M14 already holds the raw thunk address left by KVTM.
C P/PB left-shifts it by 15, producing the tagged procedural-value
C word that the rest of genEntry pushes as the actual argument.
C (P/EA performs the inverse check when adapting a formal to
C FORTRAN: ACC>>15 must be zero there.)
C
C Caller: M13 = VJM link; M14 = thunk address.
C Returns: tagged closure word in ACC via `13,UJ`.
C===========================================================
 ,ITA,14                      . ACC := M14 (raw thunk address)
 ,ITS,7                       . push M7 (activation-chain head)
 ,ASN,64-15                   . ACC <<= 15 (install Pascal descriptor
                              .   bits above the bare address)
 15,AEX,                      . exchange with caller's stack slot
 13,UJ,                       . return tagged word in ACC
C===========================================================
C P/B7 - closure untagger for indirect calls (helper #65).
C
C Not called by name from `genEntry` source text; it is pulled in
C when the instruction list contains `macro+18`, which the peephole
C expander rewrites to `KVTM,I10` followed by `VJM,P/B7`.  That
C macro is the tail of the **indirect** call sequence in genEntry
C (call through a formal routine value `p(...)`).
C
C Complementary to P/PB: right-shifts the closure word and restores
C the working tag so the indirect jump can proceed.
C===========================================================
 P/B7:,ENTRY,
 ,ITA,13                      . ACC := M13
 ,ITS,7                       . push M7
 10,XTS,                      . push M10
 ,ASN,64+15                   . ACC >>= 15 (strip descriptor bits)
 15,ATX,                      . push ACC
 10,WTC,                      . restore working tag from saved M10
 ,UJ,                         . return
C===========================================================
C P/B6 - thunk tail trampoline (helper #63).
C
C Emitted at the end of every `genEntry`-synthesized thunk, paired
C with `KUJ,actual_routine`:
C
C   form2Insn(getHelperProc(63), allocGlobalObject(r) + KUJ);
C
C Pops the saved display register, loads the target address into
C M13 and jumps there - the last hop before the real nested routine
C body runs.
C===========================================================
 P/B6:,ENTRY,
 15,XTA,                      . ACC := word from caller's stack
 ,STI,7                       . M7 := ACC (restore display)
 ,ATI,13                      . M13 := ACC (target entry from stack)
 13,UJ,                       . jump to the nested routine
C===========================================================
C P/BP - Pascal procedural call dispatcher (helper #62).
C
C Two roles, both tied to `genEntry`:
C
C 1. **Thunk body.**  When a concrete routine has no pre-built
C    closure (`value = 0`), genEntry lays down a forward jump, then
C    at `moduleOffset` emits:
C       KVTM,I10, return_point+4
C       KVTM,I9,  is_function (0/1)
C       KVTM,I8,  074001        (magic symTab slot, see symTab init)
C       VJM,P/BP
C    followed by stack-adjust / P/B6 / KUJ glue and one `idclass`
C    ordinal per formal of the passed routine.
C
C 2. **Run-time dispatch.**  When the thunk is eventually invoked,
C    P/BP validates the closure header, calls P/RSR to walk the
C    static link chain, then indirect-jumps to the entry address
C    stored in the closure object.
C
C Closure header recognised here uses the `idclass` sentinels 3, 4, 5
C (VARID, FORMALID, FIELDID) as a magic prefix on compiler-built
C thunks - not as the class of any single formal.  Simplified closures
C (first word zero) and single-word-3 headers are also accepted.
C
C On mismatch P/BP prints
C     FORMAL PROC CALL ERROR FOR PARAMETR CALL FROM <n>
C on OUTPUT and HALTs through P/HT (*0047B).
C===========================================================
 P/BP:,ENTRY,
 12,VTM,1                     . M12 := M1 (constant table base)
 15,J+M,9                     . add to M9 frame offset
 9,MTJ,11                     . M11 := M9
 *0013B:10,XTA,                . ACC := closure[0] (descriptor tag)
 ,UZA,*0045B                  . all-zero -> short path at *0045B
 9,AEX,3                      . ACC ^= 3  (VARID sentinel)
 ,UZA,*0044B                  . word was 3 -> load entry from [2]
 10,XTA,                      . reload closure[0]
 ,UTC,*0100B.=3               . compare against VARID (=3)
 ,AEX,
 ,U1A,*0047B                  . mismatch -> formal-call error
 9,XTA,3                      . ACC := closure[3]
 ,UTC,*0101B.=4               . must be FORMALID sentinel (=4)
 ,AEX,
 ,UZA,*0040B                  . matched -> full header path
 ,UTC,*0102B.=5               . closure[4] must be FIELDID (=5)
 ,AEX,
 ,U1A,*0047B                  . else error
 15,UTM,100B                  . reserve 64-word activation frame
 ,ITA,9                       . save M9 .. M13 on the stack
 ,ITS,10
 ,ITS,11
 ,ITS,12
 ,ITS,13
 9,UTC,2                      . WT += 2 (skip saved-link word)
 10,VTM,                       . M10 := SP
 10,XTS,                      . push closure pointer
 13,VJM,P/RSR                 . walk static link; fix M7 (see p_rsr)
 10,WTC,
 ,XTA,4                       . inspect closure[4] (must be zero here)
 ,U1A,*0047B                  . non-zero -> formal-call error
 10,WTC,
 13,VJM,                       . indirect call to closure entry
 15,XTA,                      . pop saved registers
 ,STI,13
 ,STI,12
 ,STI,11
 ,STI,10
 ,ATI,9
 15,XTA,10B                    . ACC := closure[2] (alternate entry)
 15,UTM,-64                   . drop the 64-word frame
 ,UJ,*0041B                   . return to synthesized thunk glue
 *0040B:9,WTC,2                . full-header path: WT += 2
 ,XTA,
 *0041B:11,ATX,2              . push closure[2] (code address)
 11,UTM,1
 12,UTM,1
 10,UTM,1
 9,UTM,2
 ,UJ,*0013B                   . re-enter dispatcher loop
 *0044B:9,XTA,2               . single-sentinel path: entry in [2]
 ,UJ,*0041B
 *0045B:9,XTA,3               . zero-header path: check closure[3]
 ,U1A,*0047B                  . must be zero
 15,XTA,                      . pop argument
 ,UJ,P/RSR                    . static-link only, then return
 *0047B:,ITA,12               . ---- formal proc call error ----------
 15,ATX,
 13,VJM,P/WOLN
 10,VTM,*0065B.=6H FORMA
 11,VTM,*0070B.=6HERROR
 13,VJM,P/PRINT
 11,VTM,*0071B.=6H FOR
 10,VTM,5
 13,VJM,P/7A
 2,VTM,*0076B.=I0
 2,XTA,
 15,XTS,-2
 13,VJM,P/WI                  . print the offending parameter index
 11,VTM,*0072B.=6H PARAM
 10,VTM,24B
 13,VJM,P/7A
 2,XTA,1
 15,XTS,-5
 2,AOX,
 13,VJM,P/WO
 13,VJM,P/WOLN
 13,VJM,P/HT
 *0065B:,ISO,18H FORMAL PROC CALL
 *0070B:,ISO,6HERROR
 *0071B:,ISO,6H FOR
 *0072B:,ISO,24H PARAMETR CALL FROM
 *0076B:,INT,0
 ,INT,6
 *0100B:,LOG,3                 . idclass VARID sentinel
 *0101B:,LOG,4                 . idclass FORMALID sentinel
 *0102B:,LOG,5                 . idclass FIELDID sentinel
 ,END,
