 P/EQ:,NAME,DTRAN  /01.06.84/    . Multi-word relational compare (date stamp 01.06.84)
C===========================================================
C P/EQ / P/GE - compare two multi-word operands (arrays, records,
C   sets, strings) for a relational operator.  Emitted by base.pas
C   getHelperProc(89 + op) when the operand is larger than 1 word.
C   In:  M12 = pointer to operand A (first word)
C        M14 = pointer to operand B (first word)
C        M11 = 1 - size (negative word count; the VLM loop counter)
C        M13 = return link
C   Out: ACC = 1U (TRUE, via *0003B) or 0 (FALSE, via *0004B).
C   P/EQ tests equality; P/GE tests the lexicographic ordering
C   (its TRUE/FALSE polarity is finalised by the caller's `negate`
C   flag, so the family covers =, <>, <, <=, >, >=).
C===========================================================
C --- P/EQ: equal iff every word matches ---
 *0000B:12,XTA,             . ACC := mem[M12] (word of A)
 14,AEX,                    . ACC ^= mem[M14] (compare with word of B)
 ,U1A,*0004B                . words differ -> not equal (*0004B, FALSE)
 12,UTM,1                   . advance A
 14,UTM,1                   . advance B
 11,VLM,*0000B              . loop over all `size` words
 *0003B:1,XTA,10B           . all equal: ACC := [M1+8] = 1U (TRUE)
 13,UJ,                     . return
 *0004B:,XTA,               . ACC := 0 (FALSE)
 13,UJ,                     . return
C===========================================
 P/GE:,ENTRY,
C===========================================
C P/GE - lexicographic compare; equal prefixes fall through to
C   the per-word ordering test on the first differing word.
 *0005B:12,XTA,             . ACC := mem[M12] (word of A)
 14,AEX,                    . ACC ^= mem[M14] (compare with word of B)
 ,U1A,*0011B                . first differing word -> decide order (*0011B)
 12,UTM,1                   . advance A
 14,UTM,1                   . advance B
 11,VLM,*0005B              . loop over all words
 ,UJ,*0003B                 . all words equal -> TRUE (*0003B)
 *0011B:12,XTA,             . differing word: ACC := mem[M12] (A's word)
 1,AEX,24B                  . ACC ^= [M1+20] (~0U): one's-complement of A's word
 14,ARX,                    . cyclic-add mem[M14] (B's word): forms B - A
 ,U1A,*0004B                . sign of B - A selects the FALSE tail (*0004B)
 ,UJ,*0003B                 . otherwise the TRUE tail (*0003B)
 ,END,
