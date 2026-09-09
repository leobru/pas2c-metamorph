#!/usr/bin/env python3
"""Emit Monitor-80 batch passport header or job trailer (UTF-8)."""
import sys

def header(dis_lines):
    sys.stdout.write(f"user 419900зс5^\n")
    for line in dis_lines:
        sys.stdout.write(line + "\n")
    sys.stdout.write("EEB1A3\n")

def trailer():
    # Exact trailer from working Monitor-80 .b6 decks (mylib.b6): six
    # backticks, then the six-character marker ЕКОНЕЦ (leading Е).
    sys.stdout.buffer.write("*end file\n``````\n".encode("ascii"))
    sys.stdout.buffer.write(b"\xd0\x95\xd0\x9a\xd0\x9e\xd0\x9d\xd0\x95\xd0\xa6\n")

def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("header", "trailer"):
        print(f"usage: {sys.argv[0]} header <dis-line>...", file=sys.stderr)
        print(f"       {sys.argv[0]} trailer", file=sys.stderr)
        return 2
    if sys.argv[1] == "header":
        header(sys.argv[2:])
    else:
        trailer()
    return 0

if __name__ == "__main__":
    sys.exit(main())
