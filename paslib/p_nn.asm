 P/NN:,NAME,DTRAN  /01.06.84/    . Entry-procedure prologue trampoline (date stamp 01.06.84)
C===========================================================
C P/NN - the prologue emitted for an *entry procedure*: a Pascal
C   routine reachable from outside the program (compiler flag 22 -
C   a FORTRAN-callable entry or the main program).  See base.pas
C   getHelperProc(94); it is reached with
C     M14 = argCount + (the routine is a FUNCTION ? 0o1000 : 0)
C     M11 = caller's return link (from the calling VJM)
C   On the FIRST external entry it runs the one-time runtime setup
C   (P/BX builds the stack frame, P/EN sets M1 := P/1D); later
C   entries skip it.  It then copies the argCount argument words
C   from the caller into the Pascal stack frame and jumps into the
C   procedure body.
C   M8 addresses the 7-word scratch block / constants at *0036B;
C   bit 0o100000 of P/STACK is the "runtime initialised" flag.
C===========================================================
 P/STACK:,LC,1               . stack-base cell (shared with P/BX)
 P/BX:,SUBP,                 . External: build the initial stack frame
 P/EN:,SUBP,                 . External: set M1 := P/1D constant base
 8,VTM,*0036B.=0             . M8 := &scratch block
 8,ATX,                      . [M8+0] := ACC (save the entry accumulator)
 ,UTC,P/STACK                . C := P/STACK
 ,XTA,                       . ACC := P/STACK
 ,UTC,*0045B.=10 0000        . C := 0o100000
 ,AAX,                       . ACC &= 0o100000 (the "initialised" bit)
 ,UZA,*0005B                 . bit clear -> first entry, *0005B
 8,XTA,                      . else ACC := [M8+0] (restore entry ACC)
 11,UJ,                      . return via M11 (already initialised)
C --- *0005B: first external entry: one-time runtime init ---
 *0005B:,ITA,11              . ACC := M11 (caller link)
 8,ATX,1                     . [M8+1] := M11 (save return link)
 ,ITA,14                     . ACC := M14 (arg descriptor)
 8,ATX,2                     . [M8+2] := M14
 11,VJM,P/BX                 . build the stack frame (link M11)
 13,VJM,P/EN                 . set M1 := P/1D (link M13)
 8,XTA,2                     . ACC := [M8+2] (arg descriptor)
 ,ASN,64+7                   . right-shift 7 (isolate the function/high flag)
 8,ATX,3                     . [M8+3] := flag
 8,XTA,2                     . ACC := [M8+2]
 8,AAX,4                     . ACC &= [M8+4] (=0o77 -> argCount)
 8,AVX,6                     . AVX [M8+6] (=0o402, sign set): ACC := -argCount
 ,ATI,9                      . M9 := -argCount (loop counter, counts up to 0)
 ,UZA,*0033B                 . no arguments -> *0033B
 9,UTM,1                     . M9 += 1
 9,V1M,*0023B                . >1 argument -> multi-word copy at *0023B
C --- *0016B: copy the final argument, then enter the body ---
 *0016B:8,XTA,3              . ACC := [M8+3] (function flag)
 ,U1A,*0033B                 . function -> skip (result handled separately)
 ,WTC,P/STACK                . C := P/STACK
 10,VTM,                     . M10 := stack base
 10,WTC,                     . C := mem[M10] (source pointer)
 11,VTM,                     . M11 := source pointer
 11,UTM,-1                   . M11 -= 1
 ,ITA,11                     . ACC := M11
 10,ATX,                     . mem[M10] := updated pointer
 ,UJ,*0033B                  . done
C --- *0023B: copy a multi-word argument onto the Pascal stack ---
 *0023B:8,WTC,3              . C := [M8+3] (this argument's word count)
 15,UTM,3                    . SP += 3 (reserve frame slots)
 ,WTC,P/STACK                . C := P/STACK
 10,VTM,                     . M10 := stack base
 10,WTC,                     . C := mem[M10] (source pointer)
 11,VTM,                     . M11 := source pointer
 9,J+M,11                    . M11 += M9 (offset by remaining count)
 ,ITA,11                     . ACC := M11
 10,ATX,                     . mem[M10] := advanced pointer
 *0030B:11,XTA,              . inner: ACC := mem[M11] (next source word)
 15,ATX,                     . push it onto the Pascal stack
 11,UTM,1                    . M11 += 1
 9,UTM,1                     . M9 += 1
 9,V1M,*0030B                . loop over this argument's words
 ,UJ,*0016B                  . next argument
C --- *0033B: restore ACC and enter the procedure body ---
 *0033B:8,XTA,               . ACC := [M8+0] (restore entry accumulator)
 1,WTC,1                     . C := [M1+1]
 13,VTM,                     . M13 := [M1+1] (runtime link)
 8,WTC,1                     . C := [M8+1] (saved caller return link)
 ,UJ,                        . jump to it: enter the procedure body
C --- *0036B: 7-word scratch block + constants (M8 base) ---
 *0036B:,LOG,                . [M8+0] saved accumulator
 ,LOG,                       . [M8+1] saved caller return link
 ,LOG,                       . [M8+2] saved M14 (arg descriptor)
 ,LOG,                       . [M8+3] function flag / word-count scratch
 ,LOG,77                     . [M8+4] argCount mask 0o77
 ,LOG,10 0000                . [M8+5] 0o100000 (initialised bit)
 ,OCT,402                    . [M8+6] 0o402 left-packed (sign set; AVX negate seed)
 *0045B:,LOG,10 0000         . 0o100000 - the "runtime initialised" flag bit
 ,END,
