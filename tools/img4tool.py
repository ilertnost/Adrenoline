#!/usr/bin/env python3
"""IMG4 containers, as Apple ships them inside an IPSW.

An IMG4 object is DER: a SEQUENCE whose first item is the ASCII string "IM4P"
(payload), "IM4R" (restore info) or "IM4M" (manifest). An IM4P carries a type
string ("rdsk", "krnl", "ibot", "dtre", ...), a description, and an OCTET
STRING payload.

The payloads are not uniform, and that matters for the loader:

  rdsk   the 20H392 restore ramdisk is a raw APFS container (NXSB)
  krnl   an LZFSE stream (bvx2) in the 20H392 kernelcache
  dtre   a device tree blob (d00dfeed), sometimes gzipped

This tool only reads. It never writes an IMG4, because nothing in this project
has to produce one: Apple's components are consumed, never re-signed.
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass, field

MAX_DEPTH = 8
TAG_INTEGER = 0x02
TAG_OCTET_STRING = 0x04
TAG_SEQUENCE = 0x30
TAG_UTF8_STRING = 0x16


class Img4Error(ValueError):
    """The input is not a well-formed IMG4 object."""


@dataclass(frozen=True)
class Element:
    tag: int
    value: bytes


@dataclass
class Img4:
    kind: str
    type: str = ""
    description: str = ""
    payload: bytes = b""
    manifest: bytes = b""
    extra: list[Element] = field(default_factory=list)

    @property
    def is_payload(self) -> bool:
        return self.kind == "IM4P"

    def summary(self) -> str:
        parts = [f"kind={self.kind}"]
        if self.type:
            parts.append(f"type={self.type}")
        if self.description:
            parts.append(f"description={self.description!r}")
        if self.payload:
            parts.append(f"payload={len(self.payload)}")
        if self.manifest:
            parts.append(f"manifest={len(self.manifest)}")
        return " ".join(parts)


def _read_der(data: bytes, offset: int) -> tuple[int, bytes, int]:
    """Read one DER TLV. Returns (tag, value, next_offset)."""
    if offset + 2 > len(data):
        raise Img4Error(f"truncated header at {offset}")
    tag = data[offset]
    length = data[offset + 1]
    header = 2
    if length & 0x80:
        count = length & 0x7F
        if count == 0 or count > 4:
            raise Img4Error(f"unsupported length form {length:#x} at {offset}")
        if offset + 2 + count > len(data):
            raise Img4Error(f"truncated length at {offset}")
        length = int.from_bytes(data[offset + 2:offset + 2 + count], "big")
        header += count
    end = offset + header + length
    if end > len(data):
        raise Img4Error(f"declared length {length} runs past the end of the input")
    return tag, data[offset + header:end], end


def _children(body: bytes) -> list[Element]:
    out: list[Element] = []
    offset = 0
    while offset < len(body):
        tag, value, offset = _read_der(body, offset)
        out.append(Element(tag, value))
    return out


def parse(data: bytes) -> Img4:
    """Parse one IMG4 object. Raises Img4Error on anything unexpected."""
    if not data:
        raise Img4Error("empty input")

    tag, body, end = _read_der(data, 0)
    if tag != TAG_SEQUENCE:
        raise Img4Error(f"outer tag is {tag:#x}, not a SEQUENCE")
    if end != len(data):
        raise Img4Error(f"{len(data) - end} trailing bytes after the IMG4 object")

    items = _children(body)
    if not items or items[0].tag != TAG_UTF8_STRING:
        raise Img4Error("first item is not a string")

    kind = items[0].value.decode("ascii", "replace")
    if kind not in ("IM4P", "IM4M", "IM4R"):
        raise Img4Error(f"unknown object kind {kind!r}")

    img4 = Img4(kind=kind)
    if kind != "IM4P":
        img4.extra = items[1:]
        return img4

    if len(items) < 4:
        raise Img4Error("IM4P needs a type, a description and a payload")
    for expected, element in ((TAG_UTF8_STRING, items[1]),
                              (TAG_UTF8_STRING, items[2]),
                              (TAG_OCTET_STRING, items[3])):
        if element.tag != expected:
            raise Img4Error(f"unexpected tag {element.tag:#x} at position "
                            f"{items.index(element)}")

    img4.type = items[1].value.decode("ascii", "replace")
    img4.description = items[2].value.decode("ascii", "replace")
    img4.payload = items[3].value
    img4.extra = items[4:]
    if img4.extra and img4.extra[0].tag == TAG_OCTET_STRING:
        img4.manifest = img4.extra[0].value
    return img4


# Payload magics worth naming. The first bytes decide how the loader must treat
# the payload, and every one of these appears in a real 20H392 image.
MAGICS = [
    (b"bvx2", "LZFSE stream"),
    (b"bvx-", "LZFSE stream"),
    (b"\x1f\x8b", "gzip stream"),
    (b"\xd0\x0d\xfe\xed", "device tree blob"),
    (b"MZ", "PE image"),
    (b"\xcf\xfa\xed\xfe", "Mach-O 64"),
    (b"\xca\xfe\xba\xbe", "Mach-O universal"),
]

# An APFS container starts with a 32-byte object header; the magic is not at
# offset zero, which is why it cannot be a plain prefix check.
APFS_MAGIC_OFFSET = 32


def classify(payload: bytes) -> str:
    """Name the payload format by its magic, before any decompression."""
    if payload[APFS_MAGIC_OFFSET:APFS_MAGIC_OFFSET + 4] == b"NXSB":
        return "APFS container"
    for magic, name in MAGICS:
        if payload.startswith(magic):
            return name
    return "unknown"


def read(path: str) -> Img4:
    with open(path, "rb") as handle:
        return parse(handle.read())


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="img4tool", description="inspect and extract IMG4 objects")
    sub = parser.add_subparsers(dest="command", required=True)

    info = sub.add_parser("info", help="print the object's fields")
    info.add_argument("file")

    extract = sub.add_parser("extract", help="write the payload to a file")
    extract.add_argument("file")
    extract.add_argument("-o", "--output", required=True)

    args = parser.parse_args(argv)

    try:
        img4 = read(args.file)
    except (Img4Error, OSError) as error:
        print(f"img4tool: {error}", file=sys.stderr)
        return 1

    if args.command == "info":
        print(img4.summary())
        if img4.payload:
            print(f"payload format: {classify(img4.payload)}")
        for element in img4.extra:
            print(f"extra: tag={element.tag:#x} length={len(element.value)}")
        return 0

    if not img4.payload:
        print("img4tool: object has no payload", file=sys.stderr)
        return 1
    with open(args.output, "wb") as handle:
        handle.write(img4.payload)
    print(f"{args.output}: {len(img4.payload)} bytes, "
          f"{classify(img4.payload)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
