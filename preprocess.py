#!/usr/bin/env python3
"""Tiny ifdef-style preprocessor: stdin -> stdout, line numbers preserved.

Usage:  preprocess.py [VAR ...] < in > out

Recognized directive comments (line starts with optional ws then '//' or '%'):
    define VAR
    ifdef  VAR
    ifndef VAR
    else
    endif

Directive lines and any lines inside an ifdef'd-out region are replaced
with a blank line so that line numbers in the output match the input.
"""
import re
import sys

DIR = re.compile(r'^\s*(?://|%)\s*(define|ifdef|ifndef|else|endif)\b\s*(\w*)')


def main():
    defined = set(sys.argv[1:])
    stack = []  # each frame: [active, parent_active, else_seen]

    def active():
        return all(f[0] for f in stack)

    for line in sys.stdin:
        m = DIR.match(line)
        if m:
            kw, name = m.group(1), m.group(2)
            if kw == 'define':
                if active() and name:
                    defined.add(name)
            elif kw in ('ifdef', 'ifndef'):
                p = active()
                cond = (name in defined) if kw == 'ifdef' else (name not in defined)
                stack.append([p and cond, p, False])
            elif kw == 'else':
                if stack and not stack[-1][2]:
                    stack[-1][2] = True
                    if stack[-1][1]:
                        stack[-1][0] = not stack[-1][0]
            elif kw == 'endif':
                if stack:
                    stack.pop()
            sys.stdout.write('\n')
        elif active():
            sys.stdout.write(line)
        else:
            sys.stdout.write('\n')


if __name__ == '__main__':
    main()
