#!/usr/bin/env python3
"""Tiny ifdef-style preprocessor with file inclusion: stdin -> stdout.

Usage:  preprocess.py [VAR ...] < in > out

Recognized directive comments (line starts with optional ws then '//' or '%'):
    define VAR
    ifdef  VAR
    ifndef VAR
    else
    endif
    include "FILE"

Directive lines and any lines inside an ifdef'd-out region are replaced with a
blank line.

include "FILE" splices FILE inline (resolved relative to the current
directory), processing it recursively.  Around the inserted content a line
marker carrying only the new line number is emitted, in the same comment prefix
as the include directive ('// include' -> '// N', '% include' -> '% N'): '<P> 1'
before the included body and '<P> L+1' after it, so a cooperating consumer can
keep per-file line numbers.  Included lines also inherit brace rewriting from
the include directive prefix: '% include' rewrites '{' -> '_(' and '}' -> '_)';
'// include' rewrites '{' -> '<:' and '}' -> ':>'.  Include cycles are a fatal
error.
"""
import os
import re
import sys

DIR = re.compile(
    r'^\s*(//|%)\s*(define|ifdef|ifndef|else|endif|include)\b\s*(.*?)\s*$')
WORD = re.compile(r'\w*')
QUOTED = re.compile(r'"([^"]+)"')


def main():
    defined = set(sys.argv[1:])
    stack = []          # each frame: [active, parent_active, else_seen]
    including = set()    # absolute paths currently open (cycle detection)

    def active():
        return all(f[0] for f in stack)

    def convert(line, prefix):
        if prefix == '%':
            return line.replace('{', '_(').replace('}', '_)')
        return line.replace('{', '<:').replace('}', ':>')

    def process(lines, path, include_prefix=None):
        lineno = 0
        for line in lines:
            lineno += 1
            m = DIR.match(line)
            if not m:
                if active():
                    if include_prefix is None:
                        sys.stdout.write(line)
                    else:
                        sys.stdout.write(convert(line, include_prefix))
                else:
                    sys.stdout.write('\n')
                continue
            prefix, kw, arg = m.group(1), m.group(2), m.group(3)
            if kw == 'define':
                if active():
                    name = WORD.match(arg).group(0)
                    if name:
                        defined.add(name)
                sys.stdout.write('\n')
            elif kw in ('ifdef', 'ifndef'):
                name = WORD.match(arg).group(0)
                p = active()
                cond = (name in defined) if kw == 'ifdef' \
                    else (name not in defined)
                stack.append([p and cond, p, False])
                sys.stdout.write('\n')
            elif kw == 'else':
                if stack and not stack[-1][2]:
                    stack[-1][2] = True
                    if stack[-1][1]:
                        stack[-1][0] = not stack[-1][0]
                sys.stdout.write('\n')
            elif kw == 'endif':
                if stack:
                    stack.pop()
                sys.stdout.write('\n')
            elif kw == 'include':
                if not active():
                    sys.stdout.write('\n')
                    continue
                q = QUOTED.match(arg)
                if not q:
                    sys.exit('preprocess: %s:%d: bad include: %s'
                             % (path, lineno, line.rstrip()))
                fname = os.path.normpath(os.path.join(os.getcwd(), q.group(1)))
                if fname in including:
                    sys.exit('preprocess: include cycle: %s' % fname)
                try:
                    f = open(fname)
                except OSError as e:
                    sys.exit('preprocess: %s:%d: cannot open %s: %s'
                             % (path, lineno, fname, e))
                sys.stdout.write('%s 1\n' % prefix)
                including.add(fname)
                with f:
                    process(f, fname, prefix)
                including.discard(fname)
                sys.stdout.write('%s %d\n' % (prefix, lineno + 1))

    process(sys.stdin, '<stdin>')


if __name__ == '__main__':
    main()
