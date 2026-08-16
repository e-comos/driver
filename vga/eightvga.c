/*
 * E-comOS VGA Display Service
 * A simple text-mode VGA driver for the E-comOS microkernel.
 * It registers itself with the init-service and handles display requests via IPC.
 */
#include <stdint.h>
#include <slfd.h>
#include <syscall.h>

/* Service identifier for the VGA display */
#define SERVICE_VGA_DISPLAY 2

/* VGA text-mode video memory (physical address 0xB8000) */
#define VGA_BUFFER          ((volatile uint16_t*)0xB8000)
#define VGA_WIDTH           80
#define VGA_HEIGHT          25

/* Stack size for the driver (16 KB) */
#define DRIVER_STACK_SIZE   0x4000

/* Cursor position tracking */
static int cursor_x = 0;
static int cursor_y = 0;

/* Dedicated stack for the driver, placed in .bss */
static uint8_t driver_stack[DRIVER_STACK_SIZE] __attribute__((aligned(16))) __attribute__((section(".bss")));

/* System call number for address mapping */
#define SYS_ADDRESS_MAP 4

/*
 * sys_map_page - Map a physical page to a virtual address (syscall wrapper)
 */
static inline long sys_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a" (ret)
        : "a" (SYS_ADDRESS_MAP),
          "D" (vaddr), "S" (paddr), "d" (flags)
        : "rcx", "r11", "memory"
    );
    return ret;
}

/* Clear the entire VGA screen */
void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = (0x0F << 8) | ' ';  // White on black
    }
    cursor_x = 0;
    cursor_y = 0;
}

/* Write a single character to the VGA buffer, handling newlines and scrolling */
void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        const size_t index = cursor_y * VGA_WIDTH + cursor_x;
        VGA_BUFFER[index] = (0x0F << 8) | c;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        /* Simple scroll-up could be implemented here */
        cursor_y = VGA_HEIGHT - 1;
    }
}

/* Print a null-terminated string to the VGA screen */
void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

/*
 * _start_c - The actual driver entry point after the stack has been switched.
 * It registers with init-service, clears the screen, and enters the main IPC loop.
 */
void _start_c(void) {
    /* Prepare a service registration message */
    char reg_payload[32];
    *((uint32_t*)reg_payload) = SERVICE_VGA_DISPLAY;

    /* Copy the service name after the ID */
    char* name_dest = reg_payload + sizeof(uint32_t);
    const char* name_src = "vga_display";
    int i = 0;
    while (name_src[i]) {
        name_dest[i] = name_src[i];
        i++;
    }
    name_dest[i] = '\0';

    /* Send registration message to init-service (PID 1) */
    ipc_send_msg(1, 0, 1, sizeof(uint32_t) + i + 1, reg_payload);

    /* Initialise the display */
    vga_clear();
    vga_print("E-comOS VGA Display Service Initialized.\n");

    /* Main IPC service loop */
    ipc_message_t msg;
    while (1) {
        if (ipc_receive_msg(&msg, -1) == 0) {
            if (msg.size > 0) {
                vga_print((char*)msg.data);
            }
        }
    }
}

/*
 * _start - The boot entry point.
 * It switches to a larger stack and then calls _start_c.
 * This function is "naked" to avoid compiler-generated prologue/epilogue.
 */
__attribute__((section(".text.entry"), naked))
void _start(void) {
    __asm__ volatile(
        "mov %0, %%rsp\n\t"
        "call _start_c\n\t"
        :
        : "r"(driver_stack + DRIVER_STACK_SIZE)
        : "memory"
    );
}
