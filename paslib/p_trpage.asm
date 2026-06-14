 P/TRPAGE:,NAME,DTRAN  /01.06.84/    . Build the drum/page free tables (date stamp 01.06.84)
C===========================================================
C P/TRPAGE - one-time startup helper that seeds the disk/drum
C   zone allocation tables used by the file runtime.
C   It clears the runtime cursors, asks P/PAGES how much drum
C   space is available, then threads every free zone onto the
C   track free-list head at [M1+37B] (the list GETTRACK / ZONEIO
C   in p_sys.asm consume).
C   Globals (M1-relative): [M1+35B]/[M1+36B] = cursor caches,
C   [M1+37B] = track free-list head.  A page/zone is 02000 (1024)
C   words.
C===========================================================
 P/PAGES:,SUBP,             . External: report available drum pages
 PASPAGN*:,LC,1             . page count (filled by P/PAGES)
 PASDRN*:,LC,1              . drum (device) count
 PASDRUM*:,LC,1             . drum descriptor / base
 ,XTA,                      . ACC := 0
 1,ATX,37B                  . [M1+31] := 0 (clear track free-list head)
 1,ATX,36B                  . [M1+30] := 0 (clear working cursor)
 1,ATX,35B                  . [M1+29] := 0 (clear cached cursor)
 14,VJM,P/PAGES             . query drum/page availability (return M14)
 ,WTC,PASPAGN*              . C := PASPAGN* (number of pages)
 10,VTM,                    . M10 := page count (loop counter)
C --- *0004B: thread each page's zone onto the free-list ---
 *0004B:10,VZM,*0011B       . pages exhausted -> *0011B
 10,UTM,-1                  . M10 -= 1
 15,MTJ,11                  . M11 := SP (scratch link cell)
 1,XTA,36B                  . ACC := [M1+30] (running zone address)
 ,ITS,9                     . push; ACC := M9
 9,UTM,2000B                . M9 += 2000B (advance one zone = 1024 words)
 ,ITS,11                    . push; ACC := M11
 15,UTM,1                   . SP += 1
 1,ATX,36B                  . [M1+30] := updated zone address
 ,UJ,*0004B                 . loop
C --- *0011B: walk the drum table, chaining zones to [M1+37B] ---
 *0011B:,WTC,PASDRN*        . C := PASDRN* (drum count)
 10,VTM,                    . M10 := drum count (loop counter)
 ,UTC,PASDRUM*              . C := PASDRUM*
 ,XTA,                      . ACC := mem[PASDRUM*] (drum descriptor)
 1,ATX,3                    . [M1+3] := drum descriptor (running)
 14,VTM,*0030B.=37          . M14 := mask 0o37 (zone-field mask)
C --- *0014B: per-drum zone-link loop ---
 *0014B:10,VZM,*0027B       . drums exhausted -> *0027B (done)
 10,UTM,-1                  . M10 -= 1
 15,MTJ,11                  . M11 := SP
 1,XTA,37B                  . ACC := [M1+31] (current list head)
 1,XTS,3                    . push; ACC := [M1+3] (this drum descriptor)
 ,ITS,11                    . push; ACC := M11
 1,ATX,37B                  . [M1+31] := new head (this zone)
 1,XTA,3                    . ACC := [M1+3]
 14,AAX,                    . ACC &= 0o37 (isolate the zone field)
 14,AEX,                    . ACC ^= 0o37 (test against the field max)
 ,UZA,*0024B                . field full -> *0024B (carry to next unit)
 1,XTA,3                    . ACC := [M1+3]
 1,ARX,10B                  . ACC += 1U  (next zone in this unit)
 1,ATX,3                    . [M1+3] := advanced descriptor
 ,UJ,*0014B                 . loop
C --- *0024B: zone field wrapped: step the unit, reset the zone ---
 *0024B:1,XTA,3             . ACC := [M1+3]
 ,ASN,64+15                 . right-shift 15
 ,ASN,64-15                 . left-shift 15  (together: clear the low 15 bits)
 14,ARX,1                   . ACC += mem[M14+1] (unit increment 1 0000B)
 1,ATX,3                    . [M1+3] := next-unit descriptor
 ,UJ,*0014B                 . loop
 *0027B:13,UJ,              . return
 *0030B:,LOG,37             . zone-field mask 0o37
 ,LOG,1 0000               . unit increment (one unit = 0o10000)
 ,END,
