/*
 * E-comOS PS/2 Keyboard Driver
 *
 * Separate module from the VGA driver: built as its own binary and loaded by
 * init-service at 0x800000 (module cmdline "kbd").
 *
 * The kernel provides two mechanisms and no policy:
 *   - IRQ1 is unmasked and SYS_IRQ_WAIT(1) wakes this thread when a key
 *     arrives;
 *   - the TSS I/O bitmap grants this process access to ports 0x60/0x64 only.
 * Controller setup, scancode translation and the IPC protocol live here.
 *
 * Protocol (with ebts):
 *   incoming MSG_CONSOLE_READ  -> wait for one key
 *   outgoing MSG_CONSOLE_INPUT -> one byte of ASCII in data[0]
 */
#include <stdint.h>
#include <slfd.h>

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64

#define MSG_CONSOLE_READ  6
#define MSG_CONSOLE_INPUT 7

#define SYS_IRQ_WAIT     5
#define IRQ_WAIT_CLEAR   0x01

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Set-1 make codes -> ASCII.  Shift is ignored (lowercase only) for now. */
static const char keymap[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
    [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',
    [0x1A] = '[', [0x1B] = ']', [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
    [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l', [0x27] = ';',
    [0x28] = '\'', [0x29] = '`', [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b',
    [0x31] = 'n', [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/',
    [0x39] = ' ',
};

static void kbd_wait_input_empty(void) {
    for (int i = 0; i < 100000; i++)
        if (!(inb(KBD_STATUS_PORT) & 0x02))
            return;
}

static void kbd_wait_output_full(void) {
    for (int i = 0; i < 100000; i++)
        if (inb(KBD_STATUS_PORT) & 0x01)
            return;
}

/* Discard everything the controller is holding (ACKs from our own commands,
 * the config byte echo, stale keys). */
static void i8042_flush(void) {
    int guard = 256;
    while ((inb(KBD_STATUS_PORT) & 0x01) && guard--)
        (void)inb(KBD_DATA_PORT);
}

static void i8042_init(void) {
    i8042_flush();

    kbd_wait_input_empty();
    outb(KBD_STATUS_PORT, 0xAE);            /* enable keyboard interface */

    kbd_wait_input_empty();
    outb(KBD_STATUS_PORT, 0x20);            /* read config byte */
    kbd_wait_output_full();
    uint8_t cfg = inb(KBD_DATA_PORT);

    cfg |= 0x01;                            /* keyboard interrupt enable */
    cfg |= 0x40;                            /* set-1 scancode translation */

    kbd_wait_input_empty();
    outb(KBD_STATUS_PORT, 0x60);            /* write config byte */
    kbd_wait_input_empty();
    outb(KBD_DATA_PORT, cfg);

    kbd_wait_input_empty();
    outb(KBD_STATUS_PORT, 0xAE);

    /* The commands above leave ACKs behind; they would be mistaken for keys
     * and, worse, keep the output buffer full so no further IRQ1 is raised. */
    i8042_flush();
}

static long irq_wait(uint32_t irq) {
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"((long)SYS_IRQ_WAIT), "D"((long)irq),
                       "S"((long)IRQ_WAIT_CLEAR), "d"(0L)
                     : "rcx", "r11", "memory");
    return ret;
}

/* Pull one byte out of the controller if it has one. */
static int kbd_poll_byte(uint8_t* out) {
    if (!(inb(KBD_STATUS_PORT) & 0x01))
        return 0;
    *out = inb(KBD_DATA_PORT);
    return 1;
}

/* Block until one printable key is pressed, then return its ASCII code. */
static int next_key(void) {
    for (;;) {
        irq_wait(1);

        /* Drain everything the controller buffered: leaving bytes behind
         * keeps its output buffer full and stops further IRQ1 edges. */
        uint8_t sc;
        while (kbd_poll_byte(&sc)) {
            if (sc & 0x80)          /* key release */
                continue;
            char c = keymap[sc & 0x7F];
            if (c)
                return (int)c;
        }
    }
}

static void kbd_main(void) {
    ipc_message_t msg;

    i8042_init();

    for (;;) {
        if (ipc_receive_msg(&msg, -1) != 0)
            continue;
        if (msg.type != MSG_CONSOLE_READ)
            continue;

        int c = next_key();
        uint8_t ch = (uint8_t)c;
        ipc_send_msg(MSG_CONSOLE_INPUT, 0, msg.source, 1, &ch);
    }
}

__attribute__((section(".text.entry"), noreturn)) void _start(void) {
    kbd_main();
    for (;;)
        ;
}
