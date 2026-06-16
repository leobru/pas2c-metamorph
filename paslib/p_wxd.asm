 P/WX:,NAME,DTRAN  /01.06.84/
C===========================================================
C P/WX / P/WXD - this module emits an 8-char word as printable
C  ISO text.  NOTHING here is hexadecimal - the routine treats
C  the word as eight 6-bit BESM-6 character codes (the on-disk
C  packed-text representation) and writes them through P/CW.
C
C  The default entry (offset 0) takes the small integer V in
C  ACC (V must fit in 8 bits, 0..255), uses it as an index into
C  an enumeration table whose base address the caller pre-loaded
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
 ,UTC,*0024B.=:7777 7777 7777 74   . C := mask clearing the low 8 bits
 ,AAX,                       . ACC := V with its low 8 bits cleared
 ,UZA,*0005B                 . V fits in 8 bits (0..255) -> table-lookup path
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
 *0007B:,ITS,13              . push the word to print; ACC := caller's M13
 ,ITS,8                      . push M13; ACC := caller's M8
 15,ATX,                     . push caller's M8
 ,UTC,*0023B.=I8             . load constant 8
 ,XTA,
 ,NTR,3                      . R := 3 (suppress normalise/round)
 15,A-X,-4                   . ACC := 8 - field_width (A-X; field width at SP-4)
 13,VJM,P/SP                 . pad with leading spaces to the field width
 8,VTM,-7                    . loop counter -7 -> 8 passes (one per char)
 *0014B:15,XTA,-3            . reload the word from our slot
 ,ASN,64-6                   . left-shift 6 (top 6-bit char shifts out into Y)
 15,ATX,-3                   . store the shifted word back
 ,YTA,                       . ACC := the 6 bits we shifted out
 13,VJM,PASISOXT             . translate internal -> ISO
 13,VJM,P/CW                 . write that 6-bit ISO char
 8,VLM,*0014B                . VLM: ++M8; loop while M8 != 0
 15,XTA,                     . pop caller's M8 into ACC
 ,STI,8                      . M8 := caller's M8 (restore); pop M13 into ACC
 ,ATI,13                     . restore M13 from ACC
 15,UTM,-2                   . drop the word slot and the caller's width arg
 13,UJ,                      . return to the caller
 *0023B:,INT,8               . the integer constant 8
 *0024B:,OCT,7777 7777 7777 74 . OCT (left-packed) -> clears the low 8 bits; P/WX range check
 ,END,
