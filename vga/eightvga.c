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

/* VGA memory is mapped into an allowed userspace virtual address. */
#define VGA_PHYSICAL_BASE   0xB8000
#define VGA_VIRTUAL_BASE    0x8B8000
#define VGA_BUFFER          ((volatile uint16_t*)VGA_VIRTUAL_BASE)
#define VGA_WIDTH           80
#define VGA_HEIGHT          25
#define MSG_VGA_WRITE       1
#define MSG_VGA_CLEAR       2
#define MSG_VGA_WRITE_CELLS 3
#define MSG_VGA_READY       4
#define MSG_VGA_FAILED      5
#define USER_PAGE_RW_FLAGS  0x7

/* Stack size for the driver (16 KB) */
#define DRIVER_STACK_SIZE   0x4000

/* Cursor position tracking */
static int cursor_x __attribute__((section(".data"))) = 0;
static int cursor_y __attribute__((section(".data"))) = 0;

/* Keep the stack in the flat image because the loader does not load ELF BSS. */
static uint8_t driver_stack[DRIVER_STACK_SIZE] __attribute__((aligned(16))) __attribute__((section(".data"))) = {0};

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
        : "0" ((long)SYS_ADDRESS_MAP),
          "D" (vaddr), "S" (paddr), "d" (flags)
        : "rcx", "r11", "memory"
    );
    return ret;
}


/* VGA CRTC ports: the TSS I/O bitmap grants these two to this process only. */
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA  0x3D5

static inline void vga_outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Move the hardware text cursor to the tracked position so it follows
 * whatever the shell is echoing. */
static void vga_update_cursor(void) {
    uint16_t pos = (uint16_t)(cursor_y * VGA_WIDTH + cursor_x);
    vga_outb(VGA_CRTC_INDEX, 0x0F);
    vga_outb(VGA_CRTC_DATA, (uint8_t)(pos & 0xFF));
    vga_outb(VGA_CRTC_INDEX, 0x0E);
    vga_outb(VGA_CRTC_DATA, (uint8_t)((pos >> 8) & 0xFF));
}

/* Clear the entire VGA screen */
void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = (0x0F << 8) | ' ';  // White on black
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_update_cursor();
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

    vga_update_cursor();
}

/* Print a null-terminated string to the VGA screen */
void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

static void vga_write_cells(const ipc_message_t* msg) {
    if (msg->size < 4 || ((msg->size - 4) % sizeof(uint16_t)) != 0)
        return;

    const uint16_t* payload = (const uint16_t*)msg->data;
    uint16_t x = payload[0];
    uint16_t y = payload[1];
    uint32_t count = (msg->size - 4) / sizeof(uint16_t);
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT)
        return;
    uint32_t available = (uint32_t)VGA_WIDTH - x;
    if (count > available)
        count = available;

    uint32_t row = (uint32_t)y * VGA_WIDTH;
    for (uint32_t i = 0; i < count; i++)
        VGA_BUFFER[row + x + i] = payload[i + 2];
}

static void vga_write_message(const ipc_message_t* msg) {
    for (uint32_t i = 0; i < msg->size; i++)
        vga_putchar((char)msg->data[i]);
}

/*
 * _start_c - The actual driver entry point after the stack has been switched.
 * It registers with init-service, clears the screen, and enters the main IPC loop.
 */
void _start_c(void) {
    if (sys_map_page(VGA_VIRTUAL_BASE, VGA_PHYSICAL_BASE,
                     USER_PAGE_RW_FLAGS) != 0) {
        ipc_send_msg(MSG_VGA_FAILED, 0, 1, 0, 0);
        for (;;);
    }

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
    ipc_send_msg(MSG_VGA_READY, 0, 1, 0, 0);

    /* Main IPC service loop */
    ipc_message_t msg;
    while (1) {
        if (ipc_receive_msg(&msg, -1) == 0) {
            if (msg.size <= sizeof(msg.data)) {
                switch (msg.type) {
                    case MSG_VGA_WRITE:
                        vga_write_message(&msg);
                        break;
                    case MSG_VGA_CLEAR:
                        vga_clear();
                        break;
                    case MSG_VGA_WRITE_CELLS:
                        vga_write_cells(&msg);
                        break;
                    default:
                        break;
                }
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
