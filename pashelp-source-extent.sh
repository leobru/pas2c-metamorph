#!/bin/sh
# Print the PASHELP LLLL440000 source designator for a .utxt file.
#
# The extent is measured, not computed.  Dubna makes the .bin it reads out of
# the .utxt itself, and how many 6144-byte zones that comes to is its business:
# a byte count taken here is a guess, and a guess that falls on the wrong side
# of a zone boundary leaves PASHELP asking for a zone the file does not have.
#
# So a small deck has dubna read the file for us.  It copies it to a scratch of
# its own, asking for more zones than any source has: the copy stops at the end
# of the source and reports it, having written exactly the zones that were
# there, and the size of the scratch says how many.  Running off the end is the
# point, so dubna's exit status is ignored.
#
# 1000 is that "more": 512 zones should the length be read as octal, which is
# an order over work.p2c's 55 and the largest thing here.  100 would not do --
# octal, that is 64.
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 source.utxt" >&2
    exit 2
fi

case "$1" in
*.utxt) ;;
*) echo "$0: $1 is not a .utxt file" >&2; exit 2 ;;
esac

# Dubna resolves a *file: name against the working directory, so go to the
# source and name it there.
cd "$(dirname "$1")"
src=$(basename "$1" .utxt)
# A dubna file name is at most six characters.
scratch=z$(($$ % 100000))
deck=tmp$$
trap 'rm -f "$deck" "$scratch.bin"' EXIT

cat > "$deck" << EOF
*NAME extent
*disc:1/local
*file:$src,44
*file:$scratch,45,w
*copy:1000,440000,450000
*end file
EOF
dubna "$deck" > /dev/null 2>&1 || true

if [ ! -s "$scratch.bin" ]; then
    echo "$0: dubna read no zones from $src.utxt" >&2
    exit 1
fi
printf '%04o440000\n' "$(( $(wc -c < "$scratch.bin") / 6144 ))"
