.extern MCP_recovery_ioctl_memcpy_hook
.global _MCP_recovery_ioctl_memcpy_hook
_MCP_recovery_ioctl_memcpy_hook:
    .thumb
    bx pc
    nop
    .arm
    ldr r12, =MCP_recovery_ioctl_memcpy_hook
    bx r12
