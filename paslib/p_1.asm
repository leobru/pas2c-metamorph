 P/1:,NAME,DTRAN  /01.06.84/    . Runtime bootstrap / program entry (date stamp 01.06.84)
C===========================================================
C P/1 - the first code a compiled Pascal program runs.  It
C   chains the three startup steps:
C     P/BX     - build the initial stack frame,
C     P/EN     - set M1 := P/1D (the runtime constant base) and
C                initialise that block,
C     P/TRPAGE - build the drum/page free tables (only if not
C                already done).
C   Control then continues into the user program proper.
C===========================================================
 P/TRPAGE:,SUBP,             . External: build drum/page free tables
 P/BX:,SUBP,                 . External: build the initial stack frame
 P/EN:,SUBP,                 . External: set up M1 = P/1D constant base
 11,VJM,P/BX                 . build the stack frame (return link in M11)
 13,VJM,P/EN                 . set M1 := P/1D and seed the constant block (link M13)
 14,MTJ,13                   . M13 := M14 (carry the program-start link forward)
 10,XTA,2                    . ACC := mem[M10+2] (page tables already built?)
 13,U1A,                     . yes -> return via M13 into the user program
 ,UJ,P/TRPAGE                . no  -> build drum/page tables, then continue
 ,END,
