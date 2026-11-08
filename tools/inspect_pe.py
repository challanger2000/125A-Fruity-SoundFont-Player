#!/usr/bin/env python3
"""Static PE/ZIP inventory. Never loads or executes a DLL.

Python 3 standard library only. Usage:
    python tools/inspect_pe.py "Fruity Soundfont Player.zip"

No proprietary bytes are copied into the repository.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import zipfile


def u16(buf, offset):
    if offset < 0 or offset + 2 > len(buf):
        raise ValueError("truncated PE word")
    return struct.unpack_from("<H", buf, offset)[0]


def u32(buf, offset):
    if offset < 0 or offset + 4 > len(buf):
        raise ValueError("truncated PE dword")
    return struct.unpack_from("<I", buf, offset)[0]


def inspect_pe(data):
    if data[:2] != b"MZ":
        raise ValueError("not an MZ executable")
    peoff = u32(data, 0x3C)
    if peoff + 24 > len(data) or data[peoff:peoff + 4] != b"PE\\0\\0":
        raise ValueError("invalid PE signature")
    coff = peoff + 4
    machine = u16(data, coff)
    count = u16(data, coff + 2)
    opt_size = u16(data, coff + 16)
    opt = coff + 20
    magic = u16(data, opt)
    if magic not in (0x10B, 0x20B):
        raise ValueError("invalid optional header")
    sections_start = opt + opt_size
    if sections_start + count * 40 > len(data):
        raise ValueError("truncated section table")
    dd_offset = opt + (96 if magic == 0x10B else 112)
    if dd_offset + 8 > sections_start:
        raise ValueError("missing export directory")
    export_rva, export_size = u32(data, dd_offset), u32(data, dd_offset + 4)
    sections = []
    for i in range(count):
        s = sections_start + i * 40
        sections.append((u32(data, s + 12), u32(data, s + 8),
                         u32(data, s + 20), u32(data, s + 16)))

    def rva_to_offset(rva, size=1):
        for va, virt, raw, rawsize in sections:
            if va <= rva and rva + size <= va + max(virt, rawsize):
                off = raw + rva - va
                if rva + size > va + rawsize or off + size > len(data):
                    raise ValueError("RVA maps outside raw section")
                return off
        raise ValueError("RVA not mapped")

    exports = []
    if export_rva:
        e = rva_to_offset(export_rva, 40)
        num_names = u32(data, e + 24)
        names_rva = u32(data, e + 32)
        if num_names > 100000:
            raise ValueError("unreasonable export count")
        t = rva_to_offset(names_rva, num_names * 4) if num_names else 0
        for i in range(num_names):
            p = rva_to_offset(u32(data, t + i * 4))
            end = data.find(b"\\0", p, min(p + 2048, len(data)))
            if end < 0:
                raise ValueError("unterminated export name")
            exports.append(data[p:end].decode("ascii", errors="replace"))
    return {
        "architecture": {0x14C: "x86", 0x8664: "x64"}.get(machine, hex(machine)),
        "pe_format": "PE32+" if magic == 0x20B else "PE32",
        "named_exports": sorted(exports),
        "export_directory_size": export_size,
    }


def analyze(path):
    def describe(name, data):
        out = {"path": name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
        if name.lower().endswith(".dll"):
            try:
                out.update(inspect_pe(data))
            except ValueError as ex:
                out["pe_error"] = str(ex)
        return out

    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            result = []
            for item in archive.infolist():
                if item.is_dir():
                    continue
                if item.file_size > 128 * 1024 * 1024:
                    raise ValueError("oversized ZIP member")
                with archive.open(item) as src:
                    data = src.read(128 * 1024 * 1024 + 1)
                if len(data) != item.file_size:
                    raise ValueError("ZIP size mismatch")
                result.append(describe(item.filename, data))
            return result
    return [describe(path.name, path.read_bytes())]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    args = parser.parse_args()
    print(json.dumps(analyze(args.source), indent=2))


if __name__ == "__main__":
    main()
