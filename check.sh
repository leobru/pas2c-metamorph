#!/bin/sh
cmp $1 $2 && echo '[1;32mSUCCESS[22;39m' || echo '[1;31mFAILURE[22;39m'
