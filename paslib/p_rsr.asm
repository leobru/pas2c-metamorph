 P/RSR:,NAME,DTRAN  /01.06.84/
C===========================================================
C P/RSR - static-link / display resolver.
C
C Called from P/BP (paslib/p_pb.asm) while dispatching a nested
C routine through a `genEntry`-synthesized procedural thunk.
C P/BP has already validated the closure header and pushed the
C closure pointer; P/RSR walks the caller's static-link chain so
C M7 ends up pointing at the enclosing activation record the
C nested routine must see.
C
C Not referenced from `helperNames`; only reached by `VJM,P/RSR`
C inside the runtime.  The compiler itself never emits a direct
C call to P/RSR - only the P/BP path installed by `genEntry`
C does.
C
C Caller convention on entry (from P/BP):
C   M13  = link from VJM
C   M10  = SP after P/BP pushed the closure pointer
C   M14  = 2 (number of static-link hops to walk)
C   M7   = current activation-chain head on entry
C   ACC  = closure pointer (also saved on the stack by P/BP)
C
C On return M7 is updated to the resolved frame base; P/BP then
C indirect-jumps through the closure entry address.
C
C The routine also contains a restore loop at *0010B that puts
C back M1/M9 state saved on the stack while unwinding.
C===========================================================
 13,UZA,                       . skip initial save if M13 = 0
 ,ATI,7                        . M7 := ACC (seed display register)
 ,ITS,1                        . push M1 (constant-table pointer)
 15,AEX,                       . ACC <-> top of stack
 13,UZA,
 7,MTJ,9                       . M9 := M7 (start of link walk)
 14,VTM,2                      . M14 := 2 (walk two nesting levels)
C ---- Walk up the static-link chain -------------------------
 *0004B:,ITA,1                 . M1 := ACC (preserve across step)
 9,AEX,2                       . ACC ^= mem[M9+2] (test link word)
 ,UZA,*0007B                   . zero -> target frame found
 9,XTA,2                       . ACC := mem[M9+2] (parent static link)
 ,ATI,9                        . M9 := ACC (move to parent frame)
 14,VLM,*0004B                 . M14 -= 1; repeat until M14 = 0
 *0007B:7,MTJ,9                . M7 := M9 (install resolved display)
C ---- Restore saved M1/M9 while popping stack slots ----------
 *0010B:,ITA,9
 14,UTC,
 ,ATI,
 ,ITA,1
 9,AEX,2
 13,UZA,
 9,XTA,2
 13,UZA,
 9,XTA,2
 ,ATI,9
 14,UTM,-1
 ,UJ,*0010B
 ,END,
