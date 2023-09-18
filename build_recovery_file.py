#!/usr/bin/env python3

from __future__ import annotations
import sys, struct

class RecoverySection:
    data: bytes
    vaddr: int
    paddr: int
    offset: int

    def __init__(self, file: str, vaddr: int, paddr: int) -> None:
        self.vaddr = vaddr
        self.paddr = paddr
        with open(file, 'rb') as f:
            self.data = f.read()

    def pack(self) -> bytes:
        return struct.pack('>IIII', self.vaddr, self.paddr, len(self.data), self.offset)

class RecoveryFile:
    MAX_RECOVERY_SECTIONS = 14
    RECOVERY_HEADER_SIZE = 0xec

    entry: int
    sections: list[RecoverySection]
    cur_offset: int

    def __init__(self, entry: int) -> None:
        self.entry = entry
        self.sections = []
        self.cur_offset = self.RECOVERY_HEADER_SIZE

    def add_section(self, section: RecoverySection) -> None:
        if len(self.sections) >= self.MAX_RECOVERY_SECTIONS:
            raise RuntimeError('too many sections')

        # set and increase section offset
        section.offset = self.cur_offset
        self.cur_offset += len(section.data)

        self.sections.append(section)

    def pack(self) -> bytes:
        packed = b''

        # magic
        packed += b'REC\0'

        # entrypoint and numSections
        packed += struct.pack('>II', self.entry, len(self.sections))

        # append section headers
        for s in self.sections:
            packed += s.pack()

        # pad the rest of the section structs
        for _ in range(self.MAX_RECOVERY_SECTIONS - len(self.sections)):
            packed += struct.pack('>IIII', 0, 0, 0, 0)

        # append section data
        for s in self.sections:
            packed += s.data

        return packed

def main(argc, argv):
    if argc != 2:
        sys.exit(-1)

    # create new recovery file
    file = RecoveryFile(0x08136000)

    # add sections
    file.add_section(RecoverySection('ios_kernel/ios_kernel.bin', 0x08136000, 0x08136000))
    file.add_section(RecoverySection('ios_mcp/ios_mcp.bin', 0x05116000, 0x05116000 - 0x05100000 + 0x13d80000))

    # write file
    with open(argv[1], 'wb') as f:
        f.write(file.pack())

if __name__ == '__main__':
    main(len(sys.argv), sys.argv)
