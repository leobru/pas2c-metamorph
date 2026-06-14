 P/BX:,NAME,DTRAN  /01.06.84/    . Build the initial stack frame + the M1 constant block (01.06.84)
C===========================================================
C P/BX / P/EN - runtime register and constant-block setup.
C
C This module owns three load-time data cells:
C   RGEXPORT - 1-word register-export save cell
C   P/1D     - the runtime CONSTANT BLOCK that M1 points to
C              (reserved 40B = 32 words).  Every [M1+n] constant
C              used across the runtime lives here; it is seeded
C              at load time from the static image at /0026B below
C              (copied to P/1D+6, so image word k lands at
C              [M1+6+k]) and finalised by P/EN.
C   P/STACK  - 1-word stack-base pointer cell
C
C P/BX builds the initial BESM-6 stack frame and saves the entry
C registers.  P/EN then points M1 at P/1D (`1,VTM,P/1D`) and
C mirrors it into M7, so the rest of the runtime can reach its
C constants as [M1+n].
C===========================================================
 RGEXPORT:,LC,1              . register-export save cell
 P/1D:,LC,40                 . runtime constant block (M1 base), 40B = 32 words
 P/STACK:,LC,1               . stack-base pointer cell
 9,VTM,P/STACK               . M9 := &P/STACK
 ,XTA,17B                    . ACC := mem[17B] (caller SP slot)
 ,ATI,10                     . M10 := that pointer
 ,ITA,15                     . ACC := M15 (current SP)
 10,MTJ,15                   . M15 := M10 (switch to the runtime stack)
 ,XTS,17B                    . push ACC; ACC := mem[17B]
 9,XTS,                      . push; ACC := mem[M9] = P/STACK
 ,ITS,13                     . push; ACC := M13 (entry link)
 12,VTM,-6                   . M12 := -6 (count of saved index registers)
 *0005B:12,ITS,7             . loop: push ACC; ACC := M7
 12,VLM,*0005B               . VLM: repeat for all six registers
 ,XTS,                       . push
 ,ITS,10                     . push; ACC := M10
 ,UTC,*0025B.=10 0000        . C := 0o100000
 ,AOX,                       . ACC |= that bit (mark the frame)
 9,ATX,                      . P/STACK := ACC (record the stack base)
 ,NTR,3                      . R := 3 (suppress normalise/round)
 11,UJ,                      . return via M11
C===========================================
 P/EN:,ENTRY,
C===========================================
C P/EN - point M1 at the constant block and snapshot the entry
C   registers into it.  `1,VTM,P/1D` is where M1 is established.
 12,VTM,*0015B               . M12 := *0015B (register-save tail)
 ,ITA,12                     . ACC := M12
 1,VTM,P/1D                  . M1 := &P/1D   <-- the runtime constant base
 1,ATX,1                     . [M1+1] := ACC (stash the tail address)
 1,MTJ,7                     . M7 := M1 (mirror the base into M7)
 13,UJ,                      . return via M13
 *0015B:9,VTM,P/STACK        . M9 := &P/STACK
 9,WTC,                      . C := P/STACK
 15,VTM,13B                  . M15 := 13B
 12,VTM,7                    . M12 := 7 (save 7 registers)
 15,XTA,                     . ACC := mem[SP]
 *0020B:12,STI,              . loop: store an index register, pop
 12,UTM,-1                   . M12 -= 1
 12,V1M,*0020B               . repeat while M12 != 0
 ,STI,13                     . restore M13
 9,STX,                      . store back through M9
 ,STX,17B                    . restore mem[17B]
 15,MTJ,9                    . M9 := SP
 ,ATI,15                     . M15 := ACC (restore SP)
 ,NTR,6                      . R := 6
 13,UJ,                      . return via M13
 *0025B:,LOG,10 0000         . constant 0o100000 (frame marker bit)
C===========================================================
C Load-time initialisation image for P/1D.  The loader copies
C /0026B into P/1D starting at offset 6, so image word k lands at
C [M1+6+k] (decimal).  ,LOG, words are right-justified numeric
C values; ,OCT, words are packed left-to-right (first octal digit
C is the most significant), so short ,OCT, constants denote high
C bits.  Each copy is a pair: `wordcount,SET,source` then
C `copies,,destination`.  (See the M1 constant table in FILE.md.)
C===========================================================
 ,DATA,
 /0026B:,LOG,                . [M1+6]  0
 ,ISO,6H′60′′60′′60′′60′′60′′60′  . [M1+7]  six '0' chars (number-format fill)
 ,LOG,1                      . [M1+8]  1U (the 1-bit unit for ARX/AOX/AEX)
 ,INT,0                      . [M1+9]  integer exponent / tag mask (bits 0,1,3)
 ,OCT,24                     . [M1+10] multiplication mask (OCT, left-packed)
 ,LOG,7757 7777 7777 7777    . [M1+11] MAXREAL (all ones without the sign bit)
 ,LOG,17 7777 7777 7777      . [M1+12] positive-mantissa mask (bits 7..47)
 ,REAL,1.E-06                . [M1+13] real 1.0e-6
 ,REAL,1.                    . [M1+14] real 1.0
 ,INT,-1                     . [M1+15] minus 1 (for AVX / end-around work)
 ,LOG,7 7777                 . [M1+16] 77777B (15-bit address mask)
 ,INT,1                      . [M1+17] integer 1
 ,ISO,6H′′′7′′7′′7′′7′′7′     . [M1+18] chars \0\7\7\7\7\7
 ,REAL,5.E-01                . [M1+19] real 0.5
 ,LOG,7777 7777 7777 7777    . [M1+20] all-ones (~0U)
 ,OCT,4                      . [M1+21] MSB, bit 48 (OCT, left-packed)
 ,LOG,17 7777 7777 7700      . [M1+22] mantissa without the 6 low bits
 /0047B:,LOG,7 6000          . [M1+27B] HEAPPTR init (reset to SP by P/GD)
 ,OCT,1                      . [M1+30B] HEAPLIM init
 /0051B:,LOG,1               . RGEXPORT init value
 17,SET,/0026B               . loader: source = 17 words at /0026B
 1,,P/1D+6                   .   copy them (x1) to P/1D+6
 2,SET,/0047B                . loader: source = 2 words at /0047B
 1,,P/1D+27                  .   copy them (x1) to P/1D+27B (HEAPPTR/HEAPLIM)
 1,SET,/0051B                . loader: source = 1 word at /0051B
 1,,RGEXPORT                 .   copy it (x1) to RGEXPORT
 ,END,
