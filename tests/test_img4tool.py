#!/usr/bin/env python3
"""Tests for tools/img4tool.py.

The fixtures are synthetic: Apple's images are never committed, so the parser
is exercised against hand-built DER that mirrors the real layout, and against
real files only when the caller points at them.
"""

import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
SPEC = importlib.util.spec_from_file_location("img4tool", ROOT / "tools" / "img4tool.py")
img4tool = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = img4tool
SPEC.loader.exec_module(img4tool)


def der(tag: int, value: bytes) -> bytes:
    length = len(value)
    if length < 0x80:
        header = bytes([tag, length])
    else:
        raw = length.to_bytes((length.bit_length() + 7) // 8, "big")
        header = bytes([tag, 0x80 | len(raw)]) + raw
    return header + value


def im4p(kind: str, payload: bytes, description: str = "0") -> bytes:
    body = (der(0x16, kind.encode())
            + der(0x16, b"rdsk")
            + der(0x16, description.encode())
            + der(0x04, payload))
    return der(0x30, body)


class ParseTests(unittest.TestCase):
    def test_reads_the_fields_of_a_payload_object(self):
        raw = im4p("IM4P", b"NXSB" + bytes(60), description="ramdisk")
        parsed = img4tool.parse(raw)

        self.assertEqual(parsed.kind, "IM4P")
        self.assertEqual(parsed.type, "rdsk")
        self.assertEqual(parsed.description, "ramdisk")
        self.assertEqual(parsed.payload[:4], b"NXSB")
        self.assertEqual(len(parsed.payload), 64)

    def test_handles_long_form_lengths(self):
        payload = bytes(range(256)) * 600           # 153 600 bytes, four digits
        parsed = img4tool.parse(im4p("IM4P", payload))
        self.assertEqual(parsed.payload, payload)

    def test_keeps_a_manifest_that_follows_the_payload(self):
        raw = der(0x30, der(0x16, b"IM4P") + der(0x16, b"krnl")
                  + der(0x16, b"0") + der(0x04, b"bvx2")
                  + der(0x04, b"\x30\x03\x16\x01M"))
        parsed = img4tool.parse(raw)
        self.assertEqual(parsed.manifest[-1:], b"M")

    def test_rejects_an_empty_input(self):
        with self.assertRaises(img4tool.Img4Error):
            img4tool.parse(b"")

    def test_rejects_a_non_sequence_outer_tag(self):
        with self.assertRaises(img4tool.Img4Error):
            img4tool.parse(der(0x04, b"IM4P"))

    def test_rejects_trailing_bytes(self):
        with self.assertRaises(img4tool.Img4Error):
            img4tool.parse(im4p("IM4P", b"x") + b"\x00\x00")

    def test_rejects_a_length_that_runs_past_the_end(self):
        raw = bytearray(im4p("IM4P", b"x" * 8))
        raw[1] = 0x7F                            # claim far more than is there
        with self.assertRaises(img4tool.Img4Error):
            img4tool.parse(bytes(raw))

    def test_rejects_an_unknown_object_kind(self):
        raw = der(0x30, der(0x16, b"IM4X") + der(0x16, b"rdsk")
                  + der(0x16, b"0") + der(0x04, b"x"))
        with self.assertRaises(img4tool.Img4Error):
            img4tool.parse(raw)


class ClassifyTests(unittest.TestCase):
    def test_names_the_formats_that_occur_in_an_ipsw(self):
        cases = {
            bytes(32) + b"NXSB" + bytes(4): "APFS container",
            b"bvx2" + bytes(4): "LZFSE stream",
            b"\x1f\x8b\x08\x00": "gzip stream",
            b"\xd0\x0d\xfe\xed": "device tree blob",
            b"\xcf\xfa\xed\xfe": "Mach-O 64",
        }
        for payload, expected in cases.items():
            with self.subTest(expected):
                self.assertEqual(img4tool.classify(payload), expected)

    def test_calls_anything_else_unknown(self):
        self.assertEqual(img4tool.classify(b"????"), "unknown")


class RealFileTests(unittest.TestCase):
    """Runs only when the caller supplies an extracted component."""

    def test_restore_ramdisk_is_an_apfs_container(self):
        path = pathlib.Path(sys.argv[0]).parent
        candidate = pathlib.Path("/data/opencode/ipsw/dmg/087-86719-047.dmg")
        if not candidate.exists():
            self.skipTest("no extracted RestoreRamDisk")
        parsed = img4tool.read(str(candidate))
        self.assertEqual(parsed.type, "rdsk")
        self.assertEqual(img4tool.classify(parsed.payload), "APFS container")
        self.assertEqual(len(parsed.payload), 26112 * 4096)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]] + [a for a in sys.argv[1:] if not a.startswith("--")])
