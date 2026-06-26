 P/EO:,NAME,DTRAN  /01.06.84/
C P/EO - Pascal eof(f), helper #8.  M12=FILE (preset by compiler).
C ACC := FILE[2]; 0=eof false, non-zero=eof true (P/GF *0306B, P/PF *0537B).
C if eof: UZA skips then on 0; while not eof: U1A exits on non-zero.
 12,XTA,2                     . ACC := FILE[2]
 13,UJ,                       . return via M13
 ,END,
