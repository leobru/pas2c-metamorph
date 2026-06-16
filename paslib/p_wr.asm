 P/WR:,NAME,DTRAN  /01.06.84/
C===========================================================
C P/WR - format a REAL for output (Pascal write(r) / write(r:w)
C   / write(r:w:d)).  It computes the decimal exponent from the
C   binary one (multiply by log10(2) = *0151B), normalises the
C   mantissa into [1,10) with the powers-of-ten table (PASTENS,
C   *0166B..), rounds to the requested number of places with the
C   0.5*10^-k table below, then prints sign, integer digits, a
C   decimal point and fraction, and - in scientific form - an
C   'E', exponent sign and exponent digits.
C
C   Helpers: P/CW prints one character; P/WI prints an integer
C   field; P/TR and P/A7 are digit/number sub-formatters.
C   M14 bases the constant block at *0144B; the format characters
C   are SPACE*, PASZERO* ('0'), PASMINS* ('-'), PASPLUS* ('+')
C   and PASPERID ('.').  Stack slots (relative to the frame SP)
C   hold the running value, field width, precision and digit
C   counts.
C===========================================================
 SPACE*:,LC,1                . ' ' (blank) character
 PASZERO*:,LC,1              . '0' character
 PASMINS*:,LC,1              . '-' character
 PASPERID:,LC,1              . '.' character (set from /0212B below)
 PASPLUS*:,LC,1              . '+' character (set from /0213B below)
 P/CW:,SUBP,                 . External: print one character
 P/TR:,SUBP,                 . External: digit / number sub-formatter
 P/A7:,SUBP,                 . External: number sub-formatter
 P/WI:,SUBP,                 . External: print an integer field
C --- prologue: take |value|, save state, base M14 on the constants ---
 ,NTR,                       . R := 0
 ,AVX,                       . absolute value / capture the sign
 ,NTR,3                      . R := 3 (suppress normalise/round)
 ,ITS,13                     . push value; ACC := M13 (save return link)
 ,XTS,                       . reserve a frame slot
 14,BASE,*0144B              . M14 = base register for the constant block
 ,XTS,                       . reserve another slot
 15,UTM,3                    . SP += 3 (finish the frame)
C --- *0004B: parse the w:d field-width / precision parameters ---
 *0004B:,XTA,*0144B          . ACC := 10
 15,A-X,-7                   . compare with the width arg [SP-7]
 ,UZA,*0012B                 . default width -> *0012B
 ,A+X,*0144B                 . add 10 back
 ,UZA,*0011B                 . -> *0011B
 ,AVX,*0145B                 . AVX with -9
 15,ATX,-7                   . store the adjusted width
 1,XTA,10B                   . ACC := [M1+8] = 1U
 15,ATX,-4                   . [SP-4] := 1
 ,UJ,*0004B                  . loop
 *0011B:,XTA,*0144B          . ACC := 10
 ,UJ,*0014B
 *0012B:,A+X,*0145B          . ACC += -9
 ,U1A,*0015B
 1,XTA,21B                   . ACC := [M1+17] = 1
 *0014B:15,ATX,-7            . [SP-7] := width
 *0015B:15,XTA,-8            . ACC := [SP-8]
 15,WTC,-4                   . C := [SP-4]
 ,A-X,*0146B                 . compare with 8
 ,UZA,*0021B
 15,WTC,-4
 ,XTA,*0146B                 . ACC := 8
 15,ATX,-8                   . [SP-8] := 8
C --- *0021B: derive the count of leading spaces to emit ---
 *0021B:15,XTA,-8
 15,A-X,-7
 15,ATX,-8
 15,XTA,-4
 ,U1A,*0132B
C --- *0024B: emit the leading spaces ---
 *0024B:15,XTA,-8            . remaining space count
 ,A-X,*0146B                 . - 8
 ,U1A,*0032B                 . none left -> sign phase (*0032B)
 ,UTC,SPACE*
 ,XTA,                       . ACC := ' '
 13,VJM,P/CW                 . print a space
 14,VTM,*0144B               . restore M14 base
 15,XTA,-8
 1,A-X,21B                   . count -= 1
 15,ATX,-8
 ,UJ,*0024B                  . loop
C --- *0032B: sign - print '-' for a negative value, else a space ---
 *0032B:15,XTA,-6
 ,NTR,20B                    . R := 020 (test the sign bit)
 ,U1A,*0036B                 . negative -> *0036B (emit '-')
 ,UTC,SPACE*
 ,XTA,                       . ACC := ' '
 ,UJ,*0040B
 *0035B:1,XTA,21B            . ACC := 1
 ,UJ,*0046B
 *0036B:,AVX,*0145B          . make the value positive
 15,ATX,-6
 ,UTC,PASMINS*
 ,XTA,                       . ACC := '-'
 *0040B:13,VJM,P/CW          . print the sign character
 14,VTM,*0144B               . restore M14 base
 15,XTA,-6
 ,UZA,*0035B                 . value == 0 -> *0035B
C --- decimal exponent: dexp = bexp * log10(2), then scale ---
 ,ASN,64+41                  . right-shift 41 -> isolate the binary exponent
 ,A-X,*0150B                 . subtract the bias (0100 = 64)
 1,AEX,11B                   . tag as integer
 ,NTR,                       . R := 0
 ,A*X,*0151B                 . * log10(2) -> decimal exponent
 ,NTR,3                      . R := 3
 1,A+X,11B                   . re-tag
 *0046B:15,ATX,-8            . save the decimal exponent
 ,AMX,                       . |ACC| - |mem[0]|
 ,ATI,13                     . M13 := that
 14,J+M,13                   . M13 += M14
 15,XTA,-8
 ,NTR,20B
 ,U1A,*0053B                 . negative exponent -> *0053B
 15,XTA,-6
 13,A/X,22B                  . divide by a power of ten
 ,UJ,*0054B
 *0053B:15,XTA,-6
 13,A*X,22B                  . multiply by a power of ten
 *0054B:15,ATX,-6            . normalised mantissa
 ,A-X,*0166B                 . compare with 1.0
 ,UZA,*0061B
 15,XTA,-6
 ,A*X,*0167B                 . * 10 (renormalise up)
 15,ATX,-6
 15,XTA,-8
 ,NTR,3
 1,A-X,21B                   . exponent -= 1
 15,ATX,-8
C --- *0061B: round to the requested precision, extract digits ---
 *0061B:13,VJM,*0134B        . add the 0.5*10^-d rounding constant
 ,X-A,*0167B                 . compare with 10
 ,UZA,*0067B
 15,XTA,-6
 ,A/X,*0167B                 . / 10 (carry out of the round)
 15,ATX,-6
 15,XTA,-8
 ,NTR,3
 1,A+X,21B                   . exponent += 1
 15,ATX,-8
 *0067B:1,XTA,21B            . ACC := 1
 15,ATX,-3                   . [SP-3] := 1 (digit position)
C --- *0070B: emit the significant digits ---
 *0070B:15,XTA,-6            . current value
 13,VJM,P/TR                 . extract/print the leading digit
 15,ATX,-2                   . save the remaining fraction
 ,NTR,
 15,X-A,-6                   . digit = value - fraction*...
 ,AMX,
 15,WTC,-7
 ,A*X,*0166B                 . * 1.0
 ,NTR,3
 1,A+X,11B                   . integer-tag
 1,ATX,4                     . [M1+4] := digit
 15,XTA,-2
 1,AEX,11B
 ,U1A,*0104B                 . -> integer-print path (*0104B)
 15,XTA,-6
 ,NTR,20B                    . test the value's sign
 ,UZA,*0104B                 . value >= 0 -> normal P/WI path
 15,XTA,-3                   . (value < 0 and no significant digit:)
 11,VTM,*0143B               . M11 := the "-0" template
 10,VTM,2                    . M10 := 2
 15,ATX,
 13,VJM,P/A7                 . print "-0" via P/A7
 ,UJ,*0106B
 *0104B:15,XTA,-3
 15,XTS,-3
 13,VJM,P/WI                 . print the integer part
C --- *0106B: decimal point, fraction, then the 'E' exponent ---
 *0106B:,UTC,PASPERID
 ,XTA,                       . ACC := '.'
 13,VJM,P/CW                 . print the decimal point
 13,VTM,SPACE*
 13,XTA,                     . ACC := ' '
 15,ATX,-1
 ,UTC,PASZERO*
 ,XTA,                       . ACC := '0'
 13,ATX,
 15,XTA,-7
 1,XTS,4
 13,VJM,P/WI                 . print the fractional digits
 15,XTA,-4
 ,U1A,*0127B                 . fixed-point only -> finish (*0127B)
 14,VTM,*0144B
 ,XTA,*0152B                 . ACC := "     E" (exponent marker)
 13,VJM,P/CW                 . print 'E'
 14,VTM,PASPLUS*             . assume '+' exponent sign
 15,XTA,-8
 ,NTR,23B                    . test the exponent sign
 ,UZA,*0124B                 . non-negative -> keep '+'
 1,AVX,17B                   . negate the exponent
 15,ATX,-8
 14,VTM,PASMINS*             . sign := '-'
 *0124B:,XTA,*0144B
 13,VJM,P/CW                 . print the exponent sign
 14,VTM,*0144B
 ,XTA,*0153B                 . ACC := 2 (exponent field width)
 15,XTS,-9
 13,VJM,P/WI                 . print the exponent digits
C --- *0127B: pad / restore / return ---
 *0127B:15,XTA,-1
 ,UTC,SPACE*
 ,ATX,                       . SPACE* := ' '  (restore)
 15,UTM,-8                   . drop the frame
 15,WTC,3                    . C := saved return address
 ,UJ,                        . return
 *0132B:15,XTA,-8
 1,A-X,21B
 15,ATX,-3
 13,VTM,*0070B
C --- *0134B: add the 0.5*10^-d rounding constant for the value ---
 *0134B:11,VTM,
 15,XTA,-6
 ,NTR,20B
 ,UZA,*0137B
 11,VTM,*0145B               . M11 := -9
 *0137B:,NTR,
 15,WTC,-7
 ,XTA,*0153B
 11,AVX,
 15,A+X,-6
 15,ATX,-6
 13,UJ,                      . return via M13
C===========================================================
C Constant block (M14-based) and rounding/scale tables.
C===========================================================
 *0143B:,ISO,2H-0            . "-0" via P/A7 when a negative value rounds to no significant digit
 *0144B:,INT,10              . 10 (decimal radix)
 *0145B:,INT,-9
 *0146B:,INT,8
 ,INT,4
 *0150B:,LOG,100             . 0100 = 64 (the exponent bias)
 *0151B:,REAL,3.0102E-01     . log10(2) = 0.30103 (binary->decimal exponent)
 *0152B:,ISO,6H′′′′′′′′′′E    . "     E" - the exponent marker
 *0153B:,INT,2
 ,REAL,5.E-02                . 0.5*10^-2  - rounding constants, one per
 ,REAL,5.E-03                .   requested number of decimal places d
 ,REAL,5.E-04
 ,REAL,5.00000000001E-05
 ,REAL,5.00000000001E-06
 ,REAL,5.00000000002E-07
 ,REAL,5.E-08
 ,REAL,5.00000000001E-09
 ,REAL,5.00000000002E-10
 ,REAL,5.00000000002E-11
C===========================================
 PASTENS:,ENTRY,             . powers-of-ten table (also used elsewhere)
C===========================================
 *0166B:,REAL,1.             . 10^0
 *0167B:,REAL,1.E01          . 10^1
 ,REAL,1.E02
 ,REAL,1.E03
 ,REAL,1.E04
 ,REAL,1.E05
 ,REAL,1.E06
 ,REAL,1.E07
 ,REAL,1.E08
 ,REAL,1.E09
 ,REAL,1.E10
 ,REAL,1.E11
 ,REAL,1.E12
 ,REAL,1.E13
 ,REAL,1.E14
 ,REAL,1.E15
 ,REAL,1.E16
 ,REAL,1.E17
 ,REAL,1.E18
 ,LOG,7757 7777 7777 7777    . MAXREAL (all ones without the sign bit)
 ,DATA,
 /0212B:,ISO,6H′′′′′′′′′′.    . "         ." -> PASPERID ('.')
 /0213B:,ISO,6H′′′′′′′′′′+    . "         +" -> PASPLUS* ('+')
 1,SET,/0212B                . loader: 1 word at /0212B
 1,,PASPERID                 .   copy to PASPERID
 1,SET,/0213B                . loader: 1 word at /0213B
 1,,PASPLUS*                 .   copy to PASPLUS*
 ,END,
