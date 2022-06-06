#!/usr/bin/env python3

import sys, struct

RECOVERY_HEADER_SIZE = 0xec

def defSection(vaddr, paddr, size, offset):
    return struct.pack('>IIII', vaddr, paddr, size, offset)

def main(argc, argv):
    if argc != 2:
        sys.exit(-1)

    # magic
    data = b'REC\0'

    # entrypoint and numSections
    data += struct.pack('>II', 0x08136000, 2)

    # read kernel binary
    kernel_bin = b''
    with open("ios_kernel/ios_kernel.bin", "rb") as f:
        kernel_bin += f.read()

    # kernel section
    data += defSection(0x08136000, 0x08136000, len(kernel_bin), RECOVERY_HEADER_SIZE)

    # read mcp binary
    mcp_bin = b''
    with open("ios_mcp/ios_mcp.bin", "rb") as f:
        mcp_bin += f.read()

    # mcp section
    data += defSection(0x05116000, 0x05116000 - 0x05100000 + 0x13d80000, len(mcp_bin), RECOVERY_HEADER_SIZE + len(kernel_bin))

    # pad the rest of the section structs
    for i in range(12):
        data += defSection(0, 0, 0, 0)

    # append kernel binary
    data += kernel_bin

    # append mcp binary
    data += mcp_bin

    # write file
    with open(argv[1], 'wb') as f:
        f.write(data)

if __name__ == '__main__':
    main(len(sys.argv), sys.argv)
