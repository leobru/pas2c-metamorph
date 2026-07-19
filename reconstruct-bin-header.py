#!/usr/bin/env python3
"""
Reconstruct the two-zone Dubna library prefix stripped by extract-module.sh.

The input is a raw DMS module body, i.e. the file produced by:

    dd bs=6k skip=2 ...

The generated prefix is 2 zones (2048 BESM-6 words) and is suitable for
prepending to a PASCOMPL-style module.  The date banner is not recoverable from
the .o payload, so it defaults to a canonical 00/00/00 unless --date is given.
"""

import argparse
import re
import sys
from pathlib import Path


WORD_BYTES = 6
ZONE_BYTES = 6144
WORDS_PER_ZONE = ZONE_BYTES // WORD_BYTES
PREFIX_WORDS = 2 * WORDS_PER_ZONE
MASK48 = (1 << 48) - 1

PASCOMPL = 0o6041634357556054
PROGRAM = 0o6062574762415500


def read_word(data, idx):
    start = idx * WORD_BYTES
    return int.from_bytes(data[start:start + WORD_BYTES], "big")


def write_word(word):
    return (word & MASK48).to_bytes(WORD_BYTES, "big")


def ascii_word(text):
    raw = text.encode("ascii")
    if len(raw) != WORD_BYTES:
        raise ValueError(f"ASCII word must be exactly 6 bytes: {text!r}")
    return int.from_bytes(raw, "big")


def dms_fields(data):
    if data.startswith(b"BESM6\0"):
        data = data[6:]
    if len(data) < 3 * WORD_BYTES:
        raise ValueError("input is too short to contain a DMS header")

    word0 = read_word(data, 0)
    word1 = read_word(data, 1)
    word2 = read_word(data, 2)

    header = word0 & 0o7777
    if header != 1:
        raise ValueError(f"unsupported DMS header field {header:o}, expected 1")

    fields = {
        "symbols": word0 >> 12,
        "header": header,
        "long_symbols": word1 >> 30,
        "data": (word1 >> 15) & 0o77777,
        "set": word1 & 0o77777,
        "constants": word2 >> 30,
        "bss": (word2 >> 15) & 0o77777,
        "commands": word2 & 0o77777,
    }
    fields["memory_size"] = (
        fields["commands"] + fields["constants"] + fields["bss"]
    )
    fields["actual_len_words"] = (
        4
        + fields["commands"]
        + fields["constants"]
        + fields["symbols"]
        + fields["long_symbols"]
        + fields["data"]
        + fields["set"]
    )
    return fields, data


def parse_date(value):
    if not re.fullmatch(r"\d\d/\d\d/\d\d", value):
        raise ValueError("--date must have form DD/MM/YY")
    return value


def build_prefix(fields, date):
    words = [0] * PREFIX_WORDS
    rounded_len = (fields["actual_len_words"] + 0o37) & ~0o37

    words[0] = 0o2104000 + rounded_len
    words[2] = PASCOMPL
    words[3] = 0o2010420000000000 | (fields["memory_size"] << 15)
    words[4] = PROGRAM

    words[7] = MASK48
    words[8] = ascii_word("LIBRAR")
    words[9] = ascii_word("Y OT  ")
    words[10] = ascii_word(date[:6])
    words[11] = ascii_word(date[6:] + "    ")
    for idx in range(12, 24):
        words[idx] = ascii_word("      ")

    words[2046] = ascii_word(date[:6])
    words[2047] = int.from_bytes(date[6:].encode("ascii") + b" \0\0\0", "big")

    return b"".join(write_word(word) for word in words)


def command_header(args):
    data = Path(args.input).read_bytes()
    fields, _ = dms_fields(data)
    date = parse_date(args.date)
    sys.stdout.buffer.write(build_prefix(fields, date))


def command_wrap(args):
    data = Path(args.input).read_bytes()
    fields, body = dms_fields(data)
    date = parse_date(args.date)
    output = build_prefix(fields, date) + body
    if args.zones:
        target_size = args.zones * ZONE_BYTES
        if len(output) > target_size:
            raise ValueError(
                f"wrapped output is {len(output)} bytes, exceeds {args.zones} zones"
            )
        output += b"\0" * (target_size - len(output))
    Path(args.output).write_bytes(output)


def command_extract(args):
    data = Path(args.input).read_bytes()
    if len(data) < 2 * ZONE_BYTES:
        raise ValueError("input is too short to contain the 2-zone Dubna prefix")
    fields, body = dms_fields(data[2 * ZONE_BYTES:])
    actual_size = fields["actual_len_words"] * WORD_BYTES
    if len(body) < actual_size:
        raise ValueError(
            f"module body is {len(body)} bytes, expected at least {actual_size}"
        )
    Path(args.output).write_bytes(body[:actual_size])


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Reconstruct the two-zone Dubna .bin prefix for a raw DMS .o"
    )
    parser.add_argument(
        "--date",
        default="00/00/00",
        help="date banner to encode as DD/MM/YY (default: 00/00/00)",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    header = sub.add_parser("header", help="write only the 2-zone prefix")
    header.add_argument("input", help="raw DMS .o file")
    header.set_defaults(func=command_header)

    wrap = sub.add_parser("wrap", help="write prefix plus unchanged .o payload")
    wrap.add_argument("input", help="raw DMS .o file")
    wrap.add_argument("output", help="output .bin file")
    wrap.add_argument(
        "--zones",
        type=int,
        default=0,
        help="pad wrapped output to this many 6K zones",
    )
    wrap.set_defaults(func=command_wrap)

    extract = sub.add_parser("extract", help="extract actual-length .o from .bin")
    extract.add_argument("input", help="input .bin file with 2-zone prefix")
    extract.add_argument("output", help="output raw DMS .o file")
    extract.set_defaults(func=command_extract)

    args = parser.parse_args(argv)
    try:
        args.func(args)
    except (OSError, ValueError) as exc:
        parser.exit(1, f"{parser.prog}: {exc}\n")


if __name__ == "__main__":
    main()
