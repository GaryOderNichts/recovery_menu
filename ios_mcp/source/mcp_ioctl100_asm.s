.extern MCP_ioctl100_patch
.global _MCP_ioctl100_patch
_MCP_ioctl100_patch:
    .thumb
    ldr r0, [r7, #0xc]
    bx pc
    nop
    .arm
    ldr r12, =MCP_ioctl100_patch
    bx r12
