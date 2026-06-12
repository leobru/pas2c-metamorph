 P/WX:,NAME,DTRAN  /01.06.84/
C===========================================================
C P/WX / P/WXD - this module emits an 8-char word as printable
C  ISO text.  NOTHING here is hexadecimal - the routine treats
C  the word as eight 6-bit BESM-6 character codes (the on-disk
C  packed-text representation) and writes them through P/CW.
C
C  The default entry (offset 0) takes the small integer V in
C  ACC (V must be 0..3), uses it as an index into a 4-word
C  enumeration table whose base address the caller pre-loaded
C  into M11 (with an additional offset in M8).  Out-of-range
C  values land on "*NOTSCA*" which is then itself printed as
C  text - giving the user a recognisable diagnostic.
C
C  The named entry P/WXD just prints whatever 48-bit word the
C  caller leaves in ACC, padded on the left to a field width
C  also supplied by the caller (placed at SP-1 below P/WXD's
C  own activation frame, see the abort handler in P/SYS for
C  an example).
C===========================================================
 PASISOXT:,SUBP,             . internal-char -> ISO translator
 P/SP:,SUBP,                 . print N spaces (N in ACC)
 P/CW:,SUBP,                 . print one character (ACC in low
C                              6 bits = ISO code)
 ,ATI,14                     . M14 := V (the requested index)
 ,UTC,*0024B.=:7777 7777 7777 74
 ,AAX,                       . ACC := V & ~3 (clear low 2 bits)
 ,UZA,*0005B                 . V was 0..3 -> table-lookup path
 ,UTC,*0004B.=TEXT*NOTSCA*   . else load the "*NOTSCA*" text
 ,XTA,
 ,UJ,*0007B                  . and print it via P/WXD
 *0004B:,TEXT,8H*NOTSCA*     . the diagnostic text (8 chars)
 *0005B:14,UTC,              . WT += M14 (=V, the table index)
 8,UTC,                      . WT += M8  (caller-supplied bias)
 11,XTA,                     . ACC := mem[M11 + V + M8]
C ---- fall through into P/WXD with the looked-up word -----
C===========================================
 P/WXD:,ENTRY,
C===========================================
 *0007B:,ITS,13              . push caller's M13
 ,ITS,8                      . push caller's M8 (P/WX setup)
 15,ATX,                     . push the 48-bit word to print
 ,UTC,*0023B.=I8             . load constant 8
 ,XTA,
 ,NTR,3                      . force ACC into integer form
 15,A-X,-4                   . ACC := field_width - 8
 13,VJM,P/SP                 . pad with that many leading spaces
 8,VTM,-7                    . loop counter: 7 iterations
 *0014B:15,XTA,-3            . reload the word from our slot
 ,ASN,64-6                   . shift right 6 bits
 15,ATX,-3                   . store the shifted word back
 ,YTA,                       . ACC := the 6 bits we shifted out
 13,VJM,PASISOXT             . translate internal -> ISO
 13,VJM,P/CW                 . write that 6-bit ISO char
 8,VLM,*0014B                . VLM: ++M8; loop while M8 != 0
 15,XTA,                     . pop the (now-empty) word slot
 ,STI,8                      . pop the saved M8 back into place
 ,ATI,13                     . restore M13 from ACC
 15,UTM,-2                   . drop the remaining two frame words
 13,UJ,                      . return to the caller
 *0023B:,INT,8               . the integer constant 8
 *0024B:,OCT,7777 7777 7777 74 . mask = ~3 (used by P/WX entry)
 ,END,
