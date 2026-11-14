ENTRY(_start)

SECTIONS
{
    /* Load address of the keyboard driver module. */
    . = 0x800000;

    .text : ALIGN(4K) {
        *(.text.entry)
        *(.text*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data*)
        *(COMMON)
        *(.bss*)
    }
}
