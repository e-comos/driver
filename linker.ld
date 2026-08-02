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

    /* Initialized read-write data */
    .data : ALIGN(4K) {
        *(.data*)
    }

    /* Uninitialized data (zeroed out by the kernel or loader) */
    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss*)
    }
}
 
