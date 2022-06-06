OUTPUT_ARCH(arm)

INCLUDE "imports.ld"

SECTIONS {
    . = (0x08136000);

    .text : {
        crt0.o(.init)
        *(.text*);
    }
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data*)
    }
    .bss : {
        __kernel_bss_start = .;
        *(.bss*)
    }
    __kernel_bss_end = .;

    /DISCARD/ : {
        *(*);
    }
}

ASSERT((SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data) + SIZEOF(.bss)) < 0xa000, "ios_kernel is too big");
