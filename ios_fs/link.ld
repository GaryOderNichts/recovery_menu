OUTPUT_ARCH(arm)

INCLUDE "imports.ld"

SECTIONS {
    .text 0x107f9000 : {
        *(.text*);
        *(.rodata*);
        *(.data*);
    }

    .bss 0x11c3c000 : {
        __fs_bss_start = .;
        *(.bss*);
    }
    __fs_bss_end = .;

    /DISCARD/ : {
        *(*);
    }
}

ASSERT(SIZEOF(.text) < 0x7000, "ios_fs text is too big");
ASSERT(SIZEOF(.bss) < 0x10000, "ios_fs bss is too big");
