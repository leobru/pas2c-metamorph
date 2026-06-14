 P/MF:,NAME,DTRAN  /01.06.84/    . Pascal -> FORTRAN call boundary (date stamp 01.06.84)
C===========================================================
C P/MF / P/FM - the Pascal<->FORTRAN calling-convention bridge,
C   emitted around a call to a FORTRAN routine when the compiler
C   is in `checkFortran` mode (see base.pas getHelperProc 92/93):
C     P/MF runs just BEFORE the call, P/FM just AFTER it returns.
C
C   They switch the stack-pointer convention between Pascal and
C   FORTRAN and toggle the "in-Pascal" flag (bit 0o100000) in the
C   shared P/STACK cell - the same flag P/NN tests on entry:
C     P/MF clears it (now executing FORTRAN),
C     P/FM sets it   (back in Pascal).
C   The Pascal accumulator is preserved across the switch in the
C   runtime scratch [M1+3].  Slot 17B is the saved stack pointer.
C===========================================================
 P/STACK:,LC,1               . shared stack-base / mode cell
 1,ATX,3                     . [M1+3] := ACC (save the Pascal accumulator)
 ,ITA,15                     . ACC := M15 (the Pascal SP)
 ,XTS,17B                    . push SP; ACC := slot 17B (saved SP)
 ,UTC,*0023B.=:7777 7777 777 . C := high-33-bits mask
 ,AAX,                       . keep the high part of slot 17B
 15,AOX,                     . OR in the low part of the popped SP
 ,ATX,17B                    . slot 17B := merged FORTRAN stack pointer
 9,VTM,P/STACK               . M9 := &P/STACK
 1,XTA,1                     . ACC := [M1+1] (runtime link)
 9,WTC,                      . C := P/STACK
 ,ATX,13B                    . slot 13B := link (stash in the new frame)
 9,WTC,                      . C := P/STACK
 ,WTC,                       . C := mem[C] (chase the P/STACK link)
 15,VTM,                     . M15 := that frame (switch SP)
 9,XTA,                      . ACC := P/STACK
 ,UTC,*0022B.=7777 7777 7767 7777 . C := ~0o100000
 ,AAX,                       . clear the "in-Pascal" flag
 9,ATX,                      . P/STACK := ACC
 1,XTA,3                     . ACC := [M1+3] (restore accumulator)
 13,UJ,                      . return via M13 (now call FORTRAN)
C===========================================
 P/FM:,ENTRY,
C===========================================
C P/FM - return side: restore the Pascal stack and re-enter Pascal.
 1,ATX,3                     . [M1+3] := ACC (save the FORTRAN result/acc)
 ,WTC,17B                    . C := slot 17B (saved Pascal SP)
 15,VTM,                     . M15 := that SP (restore the Pascal stack)
 9,VTM,P/STACK               . M9 := &P/STACK
 9,XTA,                      . ACC := P/STACK
 ,UTC,*0021B.=10 0000        . C := 0o100000
 ,AOX,                       . set the "in-Pascal" flag
 9,ATX,                      . P/STACK := ACC
 ,WTC,P/STACK                . C := P/STACK
 ,XTA,13B                    . ACC := slot 13B (saved link)
 1,ATX,1                     . [M1+1] := link (restore runtime link)
 1,XTA,3                     . ACC := [M1+3] (restore accumulator)
 ,NTR,3                      . R := 3 (suppress normalise/round)
 13,UJ,                      . return via M13 (back in Pascal)
 *0021B:,LOG,10 0000         . 0o100000 - the "in-Pascal" flag bit
 *0022B:,LOG,7777 7777 7767 7777 . ~0o100000 (all ones except the flag)
 *0023B:,OCT,7777 7777 777   . high-33-bits mask (OCT, left-packed)
 ,END,
