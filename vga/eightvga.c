#include <stdint.h>
#include <slfd.h>

#define SERVICE_VGA_DISPLAY 2
#define VGA_BUFFER          ((volatile uint16_t*)0xB8000)
#define VGA_WIDTH           80
#define VGA_HEIGHT          25

// Simple cursor tracking
static int cursor_x = 0;
static int cursor_y = 0;

static void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = (0x0F << 8) | ' '; // White text on black background
    }
    cursor_x = 0;
    cursor_y = 0;
}

static void vga_putchar(char c) {
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
        // Simple scroll up could go here
        cursor_y = VGA_HEIGHT - 1;
    }
}

static void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

__attribute__((section(".text.entry")))
void _start(void) {
    // 1. Register with init.bin as the VGA display service
    // Message format matching your microkernel service protocol
    char reg_payload[32];
    *((uint32_t*)reg_payload) = SERVICE_VGA_DISPLAY;
    // Copy name after ID
    char* name_dest = reg_payload + sizeof(uint32_t);
    const char* name_src = "vga_display";
    int i = 0;
    while (name_src[i]) { name_dest[i] = name_src[i]; i++; }
    name_dest[i] = '\0';

    // Send register message to init (PID 1 or service manager)
    ipc_send_msg(1, 1 /* MSG_SERVICE_REGISTER */, 0, sizeof(uint32_t) + i + 1, reg_payload);

    vga_clear();
    vga_print("E-comOS VGA Display Service Initialized.\n");

    // 2. Main IPC service loop
    ipc_message_t msg;
    while (1) {
        if (ipc_receive_msg(&msg, 0) == 0) {
            // If message contains text to print, process it here
            if (msg.length > 0) {
                vga_print((char*)msg.data);
            }
        }
        // Yield CPU time slice if no messages are waiting
        // thread_yield();
    }
}
