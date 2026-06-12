 P/EO:,NAME,DTRAN  /01.06.84/
C===========================================================
C P/EO - "End-of-File" predicate, returns Pascal eof(f).
C
C The runtime keeps a "buffer-pending" flag in FILE[2] that
C the open / read helpers clear when the underlying stream
C is exhausted (see *0403B in P/SYS).  eof(f) is therefore
C just the inverse of that flag, hence the one-line body:
C the caller wants a non-zero result when no more input is
C available, and FILE[2] is exactly zero in that case.
C
C Caller convention:
C   M12  = FILE record base
C   M13  = link from VJM
C
C Result:
C   ACC  = FILE[2]  (0 -> EOF reached, non-zero -> data left)
C===========================================================
 12,XTA,2                     . ACC := FILE[2] (pending flag)
 13,UJ,                       . return via M13
 ,END,
