#!/bin/sh
cmp self.o work.o && echo '[1;32mSUCCESS[22;39m' || echo '[1;31mFAILURE[22;39m'
