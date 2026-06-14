 P/BEXF:,NAME,DTRAN  /01.06.84/    . Look up a built-in external file name (date stamp 01.06.84)
C===========================================================
C P/BEXF - translate a file's external name into its device
C   designator by scanning the standard external-file table.
C   In:   [M1+3] = the 8-char external file name (the FCST
C                  literal that P/CO also stored in FILE[26])
C         M13    = return address (set by the caller; the FCST
C                  decoder in p_sys.asm arms it with 13,VTM,*0070B)
C   Out:  ACC = the matching device designator word (octal
C               LLLLNNZZZZ: length / unit / zone), or 0 if the
C               name is not a known standard file.  Return via M13.
C
C The table scanned is the runtime copy PASEXFT*, seeded from the
C static defaults at /0005B below.  Each entry is two words:
C   word 0 = 8-char name,  word 1 = its designator.
C A zero name word terminates the table.
C===========================================================
 PASEXFT*:,LC,15              . runtime external-file table (15 words)
 14,VTM,PASEXFT*              . M14 := &PASEXFT* (start of the table)
 SCAN:14,XTA,                 . ACC := mem[M14] = this entry's name
 13,UZA,                      . name == 0 (table end) -> return, ACC=0 (not found)
 1,AEX,3                      . ACC ^= [M1+3] (the name we are looking up)
 ,U1A,NEXT                    . names differ -> try the next entry
 14,XTA,1                     . match: ACC := mem[M14+1] = the designator
 13,UJ,                       . return with the designator in ACC
 NEXT:14,UTM,2                . M14 += 2 (advance to the next name/designator pair)
 ,UJ,SCAN                     . loop
C===========================================================
C Static defaults for the standard external files, copied into
C PASEXFT* at load time.  Each ,TEXT, name is followed by its
C ,LOG, designator (octal LLLLNNZZZZ); the trailing bare ,LOG,
C is the zero terminator.
C===========================================================
 ,DATA,
 /0005B:,TEXT,8H*OUTPUT*      . standard output stream
 ,LOG,6 3200                  .   designator 0o632000
 ,TEXT,8H*INPUT*              . standard input stream
 ,LOG,6 2200                  .   designator 0o622000
 ,TEXT,8HPASINPUT             . alias for standard input
 ,LOG,6 2200                  .   designator 0o622000 (same as *INPUT*)
 ,TEXT,8H*RESULT*             . scratch result file
 ,LOG,272 7000 0400           .   designator 0o2727000400
 ,TEXT,8H*CHILD*              . child/overlay file
 ,LOG,271 0000 0400           .   designator 0o2710000400
 ,LOG,                        . zero word: end-of-table marker
 11,SET,/0005B                . loader: base the init image at /0005B
 1,,PASEXFT*                  . loader: seed PASEXFT* from that image
 ,END,
