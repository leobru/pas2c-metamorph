#!/usr/bin/env python3
"""Convert a .utxt source to a zone-padded ISO/KOI7 .bin (dubna file_utxt_to_iso)."""
import sys

PAGE = 6144

# ASCII / Latin-1 subset that KOI7 leaves alone for our sources.
def to_koi7_byte(ch: str) -> int:
    o = ord(ch)
    if o == 0x00A0:
        return 0x20
    if o < 0x80:
        return o
    # Rough Cyrillic map is unnecessary for current .p2c sources; keep '?'.
    return 0x3F

def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} in.utxt out.bin", file=sys.stderr)
        return 2
    src, dst = sys.argv[1], sys.argv[2]
    out = bytearray()
    with open(src, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.endswith("\n"):
                line = line[:-1]
            out.extend(to_koi7_byte(c) for c in line[:130])
            out.append(0x0A)
    out.extend(b"\0" * 11)
    aligned = (len(out) + PAGE - 1) // PAGE * PAGE
    if len(out) < aligned:
        out.extend(b"\0" * (aligned - len(out)))
    with open(dst, "wb") as f:
        f.write(out)
    zones = len(out) // PAGE
    print(zones)
    return 0

if __name__ == "__main__":
    sys.exit(main())
