#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""mtlbinfo - inspect Apple ``.metallib`` containers.

Clean-room implementation.  The layout below was derived by inspecting the
byte stream of system metallibs shipped in an iOS image.  No Apple code is
used.

Container::

    'MTLB' + header (96 bytes; u32 function count at +88)
    record*

Each record is a run of tagged chunks::

    tag : 4 bytes ascii
    len : u16le
    payload : len bytes

    NAME  name (NUL terminated)
    TYPE  1 byte  0 = vertex, 1 = fragment, 2 = kernel
    HASH  20 bytes (sha1)
    OFFT  24 bytes
    VERS  8 bytes
    MDSZ  8 bytes  (u64 module size)
    RFLT  8 bytes
    ENDT  4 bytes  (u32 total record size, no payload)

Usage:
    mtlbinfo.py FILE.metallib [--json]
"""

from __future__ import annotations

import argparse
import json
import struct
import sys

MAGIC = b"MTLB"
HEADER_LEN = 96
COUNT_OFF = 88
END = b"ENDT"
BITCODE_WRAPPER = b"\xde\xc0\x17\x0b"
BITCODE_RAW = b"\x42\x43\xc0\xde"

KIND = {0: "vertex", 1: "fragment", 2: "kernel"}


def parse(path: str) -> dict:
    with open(path, "rb") as fh:
        data = fh.read()

    if data[:4] != MAGIC:
        raise ValueError(f"{path}: not a metallib (magic {data[:4]!r})")

    declared = struct.unpack("<I", data[COUNT_OFF:COUNT_OFF + 4])[0]
    records: list[dict] = []
    pos = data.find(b"NAME", HEADER_LEN)

    while pos >= 0 and data[pos:pos + 4] == b"NAME":
        fields: dict[bytes, bytes] = {}
        start = pos
        while True:
            tag = data[pos:pos + 4]
            n = struct.unpack("<H", data[pos + 4:pos + 6])[0]
            if tag == END:
                record_size = struct.unpack("<I", data[pos + 4:pos + 8])[0]
                pos += 4 + 4
                break
            fields[tag] = data[pos + 6:pos + 6 + n]
            pos += 6 + n
            if tag == END:
                break

        if b"NAME" not in fields:
            break

        kind_byte = fields.get(b"TYPE", b"\xff")[0] if fields.get(b"TYPE") else 0xFF
        size = struct.unpack("<Q", fields[b"MDSZ"][:8])[0] if b"MDSZ" in fields else 0
        name = fields[b"NAME"].rstrip(b"\x00").decode("ascii", "replace")

        records.append({
            "name": name,
            "kind": KIND.get(kind_byte, f"0x{kind_byte:02x}"),
            "size": size,
            "hash": fields.get(b"HASH", b"")[:20].hex(),
            "record_size": record_size,
            "file_offset": start,
        })

        # Records are contiguous: ENDT is immediately followed by the next NAME.
        if pos < len(data) and data[pos:pos + 4] != b"NAME":
            nxt = data.find(b"NAME", pos)
            if nxt < 0 or nxt - pos > 64:
                break
            pos = nxt

    return {
        "file": path,
        "bytes": len(data),
        "declared_count": declared,
        "records": records,
    }


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="inspect Apple .metallib containers")
    ap.add_argument("file")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--names", action="store_true", help="print names only")
    args = ap.parse_args(argv)

    try:
        info = parse(args.file)
    except (OSError, ValueError, KeyError) as exc:
        print(f"mtlbinfo: {exc}", file=sys.stderr)
        return 1

    if args.json:
        json.dump(info, sys.stdout, indent=2)
        sys.stdout.write("\n")
        return 0

    recs = info["records"]
    if args.names:
        for r in recs:
            print(r["name"])
        return 0

    counts: dict[str, int] = {}
    for r in recs:
        counts[r["kind"]] = counts.get(r["kind"], 0) + 1

    print(f"{info['file']}")
    print(f"  bytes          : {info['bytes']}")
    print(f"  declared count : {info['declared_count']}")
    print(f"  parsed records : {len(recs)}")
    print(f"  by kind        : " + ", ".join(f"{k}={v}" for k, v in sorted(counts.items())))
    print(f"  air total      : {sum(r['size'] for r in recs)} bytes")
    for r in recs:
        print(f"  {r['size']:8d}  {r['kind']:<9}  {r['name']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
