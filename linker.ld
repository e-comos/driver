ENTRY(_start)

SECTIONS
{
    /* The virtual address where the microkernel loads this driver */
    . = 0x700000;

    /* Code section */
    .text : ALIGN(4K) {
        /* Force our custom entry point to the absolute beginning of the binary */
        *(.text.entry)
        *(.text*)
    }

    /* Read-only data (strings, constants) */
    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    /* Include zero-initialized globals in the flat image loaded by init. */
    .data : ALIGN(4K) {
        *(.data*)
        *(COMMON)
        *(.bss*)
    }
}
 
