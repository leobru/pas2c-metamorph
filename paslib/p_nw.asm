P/NW:,NAME,DTRAN  /01.06.84/    . Heap allocator NEW(p) (date stamp 01.06.84)
P/PRINT:,SUBP,                  . External: error-message printer
P/HT:,SUBP,                     . External: halt
C===========================================
C P/NW: allocate M14 words of heap memory.
C   In:   M14 = requested size in words
C   Out:  ACC = address of newly-allocated block (success)
C         or branches via P/HT after printing "NO GLOBAL MEMORY"
C   M1-relative globals (M1 = 4000000B):
C     27B (=23) HEAPPTR  bump-allocator pointer (top of used heap)
C     30B (=24) HEAPLIM  ~(SP after stack grows) - sentinel for overflow
C     31B (=25) FREELST  head of doubly-linked free-list (0 = empty)
C     32B (=26) HEAPBSE  initial heap base, saved by P/GD
C===========================================
C --- fast path: try to bump-allocate by advancing HEAPPTR ---
1,XTA,27B            . ACC := HEAPPTR (current heap top)
,ITS,14              . push old ACC (=HEAPPTR); ACC := M14 (size)
1,ARX,27B            . ACC := size + HEAPPTR = proposed new top
1,ATX,27B            . HEAPPTR := new top (provisional)
1,ARX,30B            . ACC += HEAPLIM (= ~SP); sets overflow flag if past limit
,U1A,*0004B          . overflow ⇒ slow path (rollback + free list / OOM)
15,XTA,              . else: ACC := top-of-stack = old HEAPPTR (saved by ITS)
13,UJ,               . return ACC = address of allocated block

C --- slow path: bump failed, walk the free-list looking for a fit ---
*0004B:15,XTA,       . ACC := top-of-stack = old HEAPPTR
1,ATX,27B            . HEAPPTR := old (rollback the failed bump)
12,VTM,              . M12 := 0  (prev-link cursor: 0 means "before head")
1,XTA,31B            . ACC := FREELST (head of free list)
,ATI,11              . M11 := ACC  (current cursor)
10,VTM,*0050B        . M10 := *0050B (base of mask-literal table; see below)
,ITA,14              . ACC := M14 (requested size)
1,ATX,3              . save requested size at M1+3

C --- *0010B: free-list scan loop. M11 = cursor, M12 = prev. ---
*0010B:11,VZM,*0040B . if M11 == 0 (end of free-list) ⇒ *0040B (NO GLOBAL MEMORY)
11,XTA,              . ACC := M[M11]  (header word: [reserved:18 | SIZE:15 | NEXT:15])
,ASN,64+15           . shift right 15: SIZE field [bits 18..32] now at bits [33..47]
10,AAX,              . ACC &= mask0 (low-15)  ⇒  ACC = blockSize
15,ATX,1             . save blockSize at SP+1
1,A-X,3              . ACC := blockSize - requestedSize
,U1A,*0027B          . if blockSize >= requestedSize (non-negative) ⇒ fits, jump
11,MTJ,12            . prev := cursor  (M12 := M11)
11,WTC,              . WTC: address modifier = M[M11]
11,VTM,              . M11 := M[M11] & low-15 ⇒ next-pointer (advance cursor)
,UJ,*0010B           . loop

C --- *0016B: unlink-at-head helper (called from *0046B when M12 == 0) ---
*0016B:11,XTA,       . ACC := M[M11]  (header of cur)
10,AAX,              . ACC &= mask0  ⇒  ACC = cur's NEXT field
1,ATX,31B            . FREELST := cur->next  (unlink head)
9,UJ,                . return through M9 (= caller in *0046B)

C --- *0020B: unlink the block at M11 from the free-list (mid-list case) ---
*0020B:12,VZM,*0016B . if prev (M12) is 0, head case: jump *0016B
12,XTA,              . ACC := M[M12]  (header of prev)
10,AAX,2             . ACC &= mask2 (non-NEXT)  ⇒  prev's reserved+SIZE (no NEXT)
11,XTS,              . push ACC; ACC := M[M11]  (header of cur)
10,AAX,              . ACC &= mask0  ⇒  cur's NEXT
15,AEX,              . ACC ^= top-of-stack  ⇒  prev_reserved+SIZE | cur_next
12,ATX,              . M[M12] := ACC  (prev->next := cur->next; cur unlinked)
,ASN,64+15           . right-shift 15 (extract SIZE field of new header)
,ATI,14              . M14 := scratch (re-set to M11 by caller)
12,XTA,              . ACC := M[M12] again
14,UTC,-1            . UTC: next-instruction operand = M14 - 1
12,ATX,              . M[M12 + (M14-1)] := ACC  (mirror header at block tail)
9,UJ,                . return through M9

C --- *0027B: split path. Block at M11 fits; either exact-fit or carve a tail ---
*0027B:,ITA,14       . ACC := M14 (requested size)
15,AEX,1             . ACC ^= savedBlockSize at SP+1
,UZA,*0046B          . XOR == 0  ⇒  exact fit: take *0046B (unlink whole block)
15,XTA,1             . else: ACC := blockSize (saved at SP+1)
1,A-X,3              . ACC := blockSize - requestedSize  (= remaining size)
,ASN,64-15           . left-shift 15: move remaining-size into bits [18..32]
11,XTS,              . push ACC; ACC := M[M11] (current header)
10,AAX,1             . ACC &= mask1 (non-SIZE)  ⇒  reserved + NEXT (no SIZE)
15,AEX,              . ACC ^= top-of-stack  ⇒  reserved | newSize | NEXT
11,ATX,              . M[M11] := ACC  (block in list now reflects shrunken size)
11,J+M,14            . M14 := M11 + requestedSize  (address of carved-off tail)
14,ATX,-1            . zero out tail header at [M14-1]

C --- *0035B: tail-finalization: clear and return tail address ---
*0035B:,XTA,         . ACC := 0
14,ATX,              . M[M14] := 0
1,WTC,3              . WTC: address from M[M1+3] = saved size  (next stage)
14,ATX,-1            . M[M14-1] := 0
,ITA,14              . ACC := M14 (address of allocated block)
13,UJ,               . return

C --- out-of-memory: print "NO GLOBAL MEMORY" and halt ---
*0040B:10,VTM,*0043B.=6H NO GL   . M10 := pointer to " NO GL" segment
11,VTM,*0045B.=6HEMORY           . M11 := pointer to "EMORY" segment
13,VJM,P/PRINT                   . print (M10..M11)
13,VJM,P/HT                      . halt

*0043B:,ISO,12H NO GLOBAL M      . string " NO GLOBAL M" (12 chars)
*0045B:,ISO,6HEMORY              . string "EMORY"        (6 chars)

C --- *0046B: exact-fit. Splice out the whole block and return its addr ---
*0046B:9,VJM,*0020B              . call *0020B (M12=0 case → *0016B)
11,MTJ,14                        . M14 := M11 (returned block address)
,UJ,*0035B                       . join the tail-finalization path

C===========================================
C *0050B - *0050B+3: literal table (4 mask words used via M10 = *0050B).
C  Block-header layout is [reserved:18 | SIZE:15 | NEXT:15].
C===========================================
*0050B:,LOG,7 7777                . M10+0: NEXT-mask (low 15 bits)
,LOG,7777 7700 0007 7777          . M10+1: non-SIZE mask (clears bits 18..32)
,LOG,7777 7777 7770 0000          . M10+2: non-NEXT mask (high 33 bits)
,LOG,7777 7777 7777 7776          . M10+3: all-but-bit-47
C===========================================
P/GD:,ENTRY,                     . one-shot heap initializer
C===========================================
,ITA,15              . ACC := M15 (= SP, the current top of stack)
1,ATX,27B            . HEAPPTR := SP   (empty heap starts here)
1,ATX,32B            . HEAPBSE := SP   (save base for diagnostics)
14,J+M,15            . SP += M14       (reserve M14 words of stack)
,ITA,15              . ACC := M15      (= new SP)
1,AEX,24B            . ACC ^= ALLONES  →  ACC := ~SP
1,ARX,24B            . ACC += ALLONES (= -1)
1,ARX,10B            . ACC += E1 (= +1) →  ACC := ~SP
1,ATX,30B            . HEAPLIM := ~SP  (so HEAPPTR+ARX,30B detects overflow)
,XTA,                . ACC := 0
1,ATX,31B            . FREELST := 0    (empty free-list)
13,UJ,               . return
,END,

