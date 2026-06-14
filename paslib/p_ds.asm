 P/DS:,NAME,DTRAN  /01.06.84/    . Heap deallocator DISPOSE(p) (date stamp 01.06.84)
C===========================================
C P/DS: free the heap block pointed to by p, coalescing with neighbours.
C   In:   M9  = address of the pointer variable p (an lvalue; set by SETREG9)
C         M14 = block size in words (set by KVTM+I14 before the call)
C         M13 = return address (from the calling VJM)
C   Out:  the block is returned to the bump allocator (if it sits at the top
C         of the heap) or pushed onto the free-list; control returns to M13.
C   M1-relative globals (M1 = 4000000B):
C     27B (=23) HEAPPTR  bump-allocator pointer (top of used heap)
C     31B (=25) FREELST  head of the free-list (0 = empty)
C
C Block-header layout (one word):  [TAG:18 | SIZE:15 | NEXT:15]
C   TAG  (bits 48..31) = 775317B marks a FREE block.
C   SIZE (bits 30..16) = block length in words.
C   NEXT (bits 15..1)  = link to the next free block (free blocks only).
C   A free block stores an identical copy of its header word in its LAST word
C   (a "tail mirror"), so the block physically below p can be recognised by
C   inspecting mem[p-1], and two adjacent free blocks share equal header words.
C
C Index-register usage:
C   M9  working pointer: &p -> p -> p-1 -> p+N (neighbour-header probe)
C   M10 base of the mask/literal table at *0065B (established by BASE)
C   M11 free-list scan cursor (current block)
C   M12 free-list scan previous (prev link; 0 = at head)
C   M13 return address (saved on the stack; reused as scratch in *0045B)
C   M14 size N -> p+N (end address) -> scratch
C   M15 SP
C
C Stack frame built by the prologue (offsets from SP at the merge/insert points):
C   [SP-5] block base p     (overwritten with the merged base on back-merge)
C   [SP-4] return address
C   [SP-3] size N           (overwritten with the merged size on coalesce)
C   [SP-2] p+N              (end-of-block address, exclusive)
C   [SP-1] p-1              (word just below the block)
C===========================================
C --- prologue: dereference p and build the stack frame ---
 ,NTR,3              . R := 3: suppress normalisation+rounding (raw 48-bit math)
 9,XTA,              . ACC := mem[M9] = p          (the block base address)
 ,ATI,9              . M9 := p                      (M9 now holds the block base)
 ,ITS,13             . push p;        ACC := M13 (return address)
 ,ITS,14             . push retaddr;  ACC := M14 (= N, the size)
 9,J+M,14            . M14 := M14 + M9 = p + N      (end-of-block address)
 ,ITS,14             . push N;        ACC := M14 (= p+N)
 9,UTM,-1            . M9 := p - 1                  (point at the word below the block)
 ,ITS,9              . push p+N;      ACC := M9 (= p-1)
 9,XTS,              . push p-1;      ACC := mem[p-1]  (tail-mirror of the block below)
C --- is the block physically below p a free block? (tag check on mem[p-1]) ---
 10,BASE,*0065B      . M10 := *0065B  (base register for the mask/literal table)
 ,AAX,*0066B.=7777 7700 0000 0000   . ACC &= TAG mask  (keep bits 48..31)
 ,AEX,*0065B.=7753 1700 0000 0000   . ACC ^= 775317B   (0 iff the TAG matches)
 ,U1A,*0024B         . tag mismatch (below not free) -> skip back-merge, *0024B
C --- back-merge: find the preceding free block in the list (header == mem[p-1]) ---
 1,XTA,31B           . ACC := FREELST (head of free-list)
 12,VTM,             . M12 := 0       (prev cursor = before head)
 *0010B:,ATI,11      . M11 := ACC&low15  (current free-block address / advance step)
 11,VZM,*0024B       . list exhausted (M11==0) -> give up back-merge, *0024B
 11,XTA,             . ACC := mem[M11]   (header of the current free block)
 9,AEX,              . ACC ^= mem[p-1]   (compare with the mirror we are matching)
 ,UZA,*0015B         . headers equal (ACC==0) -> candidate predecessor, *0015B
 *0013B:11,MTJ,12    . M12 := M11        (prev := cur)
 11,XTA,             . ACC := mem[M11]   (reload header; low15 -> next at *0010B)
 ,UJ,*0010B          . loop to next free block
C --- *0015B: candidate found; verify it is physically adjacent below p ---
 *0015B:11,XTA,      . ACC := mem[M11]   (predecessor header)
 ,ASN,64+15          . ACC >>= 15        (bring SIZE field down to the low bits)
 ,AAX,*0067B.=7 7777 . ACC &= low15      (ACC = predecessor SIZE)
 ,ITS,11             . push predSize;    ACC := M11 (= predecessor base)
 15,ARX,             . pop predSize;     ACC := predBase + predSize (its end address)
 15,AEX,-5           . ACC ^= p          (0 iff predecessor ends exactly at p)
 ,U1A,*0013B         . not adjacent -> false match, keep scanning at *0013B
 ,ITA,11             . ACC := M11 = predBase
 15,ATX,-5           . [SP-5] := predBase            (merged block starts at predecessor)
 15,MTJ,14           . M14 := SP                      (frame pointer for the scratch reload)
 14,XTA,             . ACC := mem[SP] = predSize      (value left in the popped slot)
 15,A+X,-3           . ACC := predSize + N            (merged size)
 15,ATX,-3           . [SP-3] := merged size
 13,VJM,*0054B       . unlink the predecessor from the free-list; return to *0024B
C --- *0024B: is the block physically above (at p+N) a free block? ---
 *0024B:15,WTC,-2    . C := (p+N)&low15
 9,VTM,              . M9 := p + N                    (header of the following block)
 9,XTA,              . ACC := mem[p+N]                (its header word)
 ,AAX,*0066B.=7777 7700 0000 0000   . ACC &= TAG mask
 ,AEX,*0065B.=7753 1700 0000 0000   . ACC ^= 775317B  (0 iff the following block is free)
 ,UZA,*0034B         . following block is free -> forward-merge at *0034B
C --- *0027B: no forward neighbour. If the block ends at HEAPPTR, just shrink it. ---
 *0027B:15,XTA,-2    . ACC := p + N
 1,AEX,27B           . ACC ^= HEAPPTR                 (0 iff the block is at the heap top)
 ,U1A,*0045B         . not at the top -> link into the free-list at *0045B
 15,XTA,-5           . ACC := block base (p or merged base)
 1,ATX,27B           . HEAPPTR := base                (return the top block to the bump heap)
C --- *0032B: epilogue. Drop the frame and return through the saved address. ---
 *0032B:15,UTM,-5    . SP -= 5                        (pop the 5-word frame)
 15,WTC,1            . C := saved return address (= old [SP-4])
 ,UJ,                . return: jump to C
C --- *0034B: forward-merge. Find the following free block in the list. ---
 *0034B:1,XTA,31B    . ACC := FREELST
 12,VTM,             . M12 := 0       (prev cursor)
 *0035B:,ATI,11      . M11 := ACC&low15  (current free block / advance step)
 11,VZM,*0027B       . list exhausted -> give up, link single block at *0027B
 11,XTA,             . ACC := mem[M11]   (current header)
 9,AEX,              . ACC ^= mem[p+N]   (match against the following block's header)
 ,UZA,*0041B         . found the following free block -> *0041B
 11,MTJ,12           . M12 := M11        (prev := cur)
 11,XTA,             . ACC := mem[M11]   (reload header)
 ,UJ,*0035B          . loop
C --- *0041B: unlink the following block and add its size to ours ---
 *0041B:13,VJM,*0054B . unlink the following block from the free-list
 9,XTA,              . ACC := mem[p+N]   (following header, still readable)
 ,ASN,64+15          . ACC >>= 15        (bring SIZE down)
 ,AAX,*0067B.=7 7777 . ACC &= low15      (following SIZE)
 15,A+X,-3           . ACC := size + followingSize
 15,ATX,-3           . [SP-3] := merged size      (then fall into *0045B)
C --- *0045B: link the (possibly merged) block at the head of the free-list ---
 *0045B:15,XTA,-3    . ACC := final size
 ,ASN,64-15          . ACC <<= 15        (place size into the SIZE field, bits 30..16)
 ,AEX,*0065B.=7753 1700 0000 0000   . ACC ^= 775317B   (OR in the free TAG)
 1,AEX,31B           . ACC ^= FREELST    (set NEXT := old FREELST)
 15,WTC,-5           . C := block base
 14,VTM,             . M14 := base
 14,ATX,             . mem[base] := header           (write the head header)
 15,XTS,-6           . push header;   ACC := base     ([SP-5] reloaded after the push)
 15,A+X,-4           . ACC := base + size             (= end address; size now at [SP-4])
 ,STI,13             . M13 := base+size; pop header back into ACC
 13,ATX,-1           . mem[base+size-1] := header     (write the tail-mirror header)
 15,XTA,-5           . ACC := base
 1,ATX,31B           . FREELST := base               (new block becomes the list head)
 ,UJ,*0032B          . join the epilogue
C===========================================
C *0054B: unlink the free block at M11 from the free-list.
C   In:  M11 = block to unlink, M12 = its predecessor in the list (0 = head).
C   Patches the predecessor's NEXT and re-mirrors the predecessor's tail header.
C===========================================
 *0054B:12,VZM,*0063B . prev == 0 -> unlink at head (*0063B)
 12,XTA,             . ACC := mem[M12]   (predecessor header)
 ,AAX,*0070B.=7777 7777 7770 0000   . ACC &= TAG+SIZE mask (clear NEXT)
 11,XTS,             . push prev(tag|size);  ACC := mem[M11] (cur header)
 ,AAX,*0067B.=7 7777 . ACC &= low15      (cur's NEXT)
 15,AEX,             . pop; ACC := prev(tag|size) | cur.next
 12,ATX,             . mem[M12] := ACC   (prev->next := cur->next; cur unlinked)
 ,ASN,64+15          . ACC >>= 15        (extract the predecessor's SIZE)
 ,ATI,14             . M14 := predecessor SIZE
 12,XTA,             . ACC := mem[M12]   (updated predecessor header)
 14,UTC,-1           . C := SIZE - 1
 12,ATX,             . mem[M12 + SIZE-1] := header  (re-mirror the predecessor's tail)
 13,UJ,              . return
 *0063B:11,XTA,      . ACC := mem[M11]   (head block header)
 ,AAX,*0067B.=7 7777 . ACC &= low15      (its NEXT)
 1,ATX,31B           . FREELST := cur->next  (unlink the head)
 13,UJ,              . return
C===========================================
C Mask/literal table, addressed via M10 = *0065B (see BASE above).
C Header layout is [TAG:18 | SIZE:15 | NEXT:15].
C===========================================
 *0065B:,LOG,7753 1700 0000 0000   . free-block TAG value (775317B in bits 48..31)
 *0066B:,LOG,7777 7700 0000 0000   . TAG-field mask        (bits 48..31)
 *0067B:,LOG,7 7777                . NEXT / SIZE low mask  (low 15 bits)
 *0070B:,LOG,7777 7777 7770 0000   . TAG+SIZE mask         (clears the NEXT field)
 ,END,
