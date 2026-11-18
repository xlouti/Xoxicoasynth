#!/usr/bin/env python3
"""Mark os_workgroup imports weak so Catalina's dyld can load the x86_64 slice.

The macOS 15 SDK resolves these macOS 11 symbols as strong imports. ld -U does
not weaken a symbol the SDK actually provides, so set N_WEAK_REF on the nlist
entry and the weak bit on any dyld bind opcode that names the symbol.
"""
import struct
import sys

N_WEAK_REF = 0x0040
MH_MAGIC_64 = 0xFEEDFACF
FAT_MAGIC = 0xCAFEBABE
LC_SYMTAB = 0x2
LC_DYLD_INFO = 0x22
LC_DYLD_INFO_ONLY = 0x80000022
BIND_OPCODE_SET_SYMBOL = 0x40


def skip_uleb(buf, i):
    while True:
        b = buf[i]
        i += 1
        if b < 128:
            return i


def weaken_bind(buf, start, size):
    """Set BIND_SYMBOL_FLAGS_WEAK_IMPORT on os_workgroup symbol opcodes."""
    end = start + size
    i = start
    changed = 0
    while i < end:
        op = buf[i]
        opc = op & 0xF0
        imm = op & 0x0F
        if opc == BIND_OPCODE_SET_SYMBOL:
            name_end = buf.index(0, i + 1)
            name = bytes(buf[i + 1:name_end])
            if name.startswith(b"_os_workgroup") and not (imm & 1):
                buf[i] = opc | (imm | 1)
                changed += 1
            i = name_end + 1
        elif opc in (0x20, 0x60, 0x70, 0x80, 0xA0):
            i = skip_uleb(buf, i + 1)
        elif opc == 0xC0:
            i = skip_uleb(buf, i + 1)
            i = skip_uleb(buf, i)
        else:
            i += 1
    return changed


def weaken_slice(buf, base):
    if struct.unpack_from("<I", buf, base)[0] != MH_MAGIC_64:
        raise SystemExit("expected a 64-bit Mach-O slice")
    ncmds = struct.unpack_from("<I", buf, base + 16)[0]
    off = base + 32
    symoff = nsyms = stroff = None
    binds = []
    for _ in range(ncmds):
        cmd, cmdsize = struct.unpack_from("<II", buf, off)
        if cmd == LC_SYMTAB:
            symoff, nsyms, stroff, _strsize = struct.unpack_from("<IIII", buf, off + 8)
            symoff += base
            stroff += base
        elif cmd in (LC_DYLD_INFO, LC_DYLD_INFO_ONLY):
            fields = struct.unpack_from("<IIIIIIIIII", buf, off + 8)
            for idx in (2, 4, 6):
                binds.append((base + fields[idx], fields[idx + 1]))
        off += cmdsize
    if symoff is None:
        raise SystemExit("missing LC_SYMTAB")
    marked = 0
    found = 0
    for i in range(nsyms):
        so = symoff + i * 16
        n_strx = struct.unpack_from("<I", buf, so)[0]
        name_at = stroff + n_strx
        name_end = buf.index(0, name_at)
        name = bytes(buf[name_at:name_end])
        if not name.startswith(b"_os_workgroup"):
            continue
        found += 1
        desc = struct.unpack_from("<H", buf, so + 6)[0]
        if not (desc & N_WEAK_REF):
            struct.pack_into("<H", buf, so + 6, desc | N_WEAK_REF)
            marked += 1
    bound = 0
    for start, size in binds:
        if size:
            bound += weaken_bind(buf, start, size)
    if found == 0:
        raise SystemExit("no os_workgroup imports in this slice")
    return marked, bound


def weaken_file(path):
    data = bytearray(open(path, "rb").read())
    magic = struct.unpack_from(">I", data, 0)[0]
    ranges = []
    if magic == FAT_MAGIC:
        nfat = struct.unpack_from(">I", data, 4)[0]
        for i in range(nfat):
            _cputype, _sub, offset, size, _align = struct.unpack_from(">IIIII", data, 8 + i * 20)
            ranges.append((offset, size))
    elif struct.unpack_from("<I", data, 0)[0] == MH_MAGIC_64:
        ranges.append((0, len(data)))
    else:
        raise SystemExit(f"not a Mach-O file: {path}")
    total_sym = 0
    total_bind = 0
    for offset, _size in ranges:
        marked, bound = weaken_slice(data, offset)
        total_sym += marked
        total_bind += bound
    open(path, "wb").write(data)
    print(f"{path}: weak-linked {total_sym} os_workgroup symbol entries, {total_bind} bind opcodes")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} <mach-o>")
    weaken_file(sys.argv[1])
