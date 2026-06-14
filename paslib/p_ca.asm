 P/CA:,NAME,DTRAN  /01.06.84/    . Checked pointer dereference (date stamp 01.06.84)
 P/ER:,SUBP,                     . External: runtime error printer ("... LINE n", aborts)
C===========================================
C P/CA: validate a heap pointer before a dereference (emitted for p@ when
C       range-checking is on; see genDeref / getHelperProc(7)).
C   In:   ACC = pointer value to check (the value of p)
C         M14 = current source line number (set by KVTM+I14+lineCnt)
C         M13 = return address (from the calling VJM)
C   Out:  valid   -> M14 := the validated pointer (the caller then dereferences
C                    through M14); ACC = pointer; SP restored; return via M13.
C         invalid -> tail-jump to P/ER with the message below; never returns.
C   A pointer is valid iff  HEAPBSE <= p < HEAPPTR  (inside the live heap).
C   M1-relative globals (M1 = 4000000B):
C     27B (=23) HEAPPTR  top of used heap (first word past the live heap)
C     32B (=26) HEAPBSE  initial heap base, saved by P/GD
C   Error-path registers handed to P/ER:
C     M11 = message address, M10 = message length (24), M14 = line number
C===========================================
 15,ATX,             . push p onto the stack (ACC unchanged)
 1,A-X,32B           . ACC := p - HEAPBSE
 ,U1A,*0005B         . p < HEAPBSE (ACC < 0) -> out of range, *0005B
 15,XTA,-1           . ACC := p           (reload the pushed pointer)
 1,A-X,27B           . ACC := p - HEAPPTR
 ,UZA,*0005B         . p >= HEAPPTR (ACC >= 0) -> out of range, *0005B
 15,XTA,             . pop p back into ACC (restore SP)
 ,ATI,14             . M14 := p           (validated pointer for the dereference)
 13,UJ,              . return
C --- out-of-range: report "POINTER OVERRANGE. LINE n" via P/ER and abort ---
 *0005B:11,VTM,*0007B.=6H POINT   . M11 := address of the message string
 10,VTM,30B          . M10 := 30B (=24) message length in characters
 ,UJ,P/ER            . tail-call the error printer (M14 still = line number)
 *0007B:,ISO,24H POINTER OVERRANGE. LINE   . 24-char message (leading space)
 ,END,
