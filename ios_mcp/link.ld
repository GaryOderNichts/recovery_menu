OUTPUT_ARCH(arm)

INCLUDE "imports.ld"

SECTIONS {
    .text 0x05116000 : {
        *(.text*);
        *(.rodata*);
        *(.data*);
    }

    .bss 0x050bd000 : {
        __mcp_bss_start = .;
        *(.bss*);
    }
    __mcp_bss_end = .;

    /DISCARD/ : {
        *(*);
    }
}

ASSERT(SIZEOF(.text) < 0x6000, "ios_mcp text is too big");
ASSERT(SIZEOF(.bss) < 0x3000, "ios_mcp bss is too big");
