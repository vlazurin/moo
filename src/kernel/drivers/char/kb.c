#include "kb.h"
#include "port.h"
#include "debug.h"
#include "vfs.h"
#include "buffer.h"
#include "interrupts.h"

#define ESC    27
#define BACKSPACE '\b'
#define TAB       '\t'
#define ENTER     '\n'
#define RETURN    '\r'
#define NEWLINE ENTER

// Non-ASCII special scancodes // Esc in scancode is 1
#define    KESC         1
#define    KF1          0x80
#define    KF2         (KF1 + 1)
#define    KF3         (KF2 + 1)
#define    KF4         (KF3 + 1)
#define    KF5         (KF4 + 1)
#define    KF6         (KF5 + 1)
#define    KF7         (KF6 + 1)
#define    KF8         (KF7 + 1)
#define    KF9         (KF8 + 1)
#define    KF10        (KF9 + 1)
#define    KF11        (KF10 + 1)
#define    KF12        (KF11 + 1)

// Cursor Keys
#define    KINS         0x90
#define    KDEL        (KINS + 1)
#define    KHOME       (KDEL + 1)
#define    KEND        (KHOME + 1)
#define    KPGUP       (KEND + 1)
#define    KPGDN       (KPGUP + 1)
#define    KLEFT       (KPGDN + 1)
#define    KUP         (KLEFT + 1)
#define    KDOWN       (KUP + 1)
#define    KRIGHT      (KDOWN + 1)

// "Meta" keys
#define    KMETA_ALT     0x0200                                // Alt is pressed
#define    KMETA_CTRL    0x0400                                // Ctrl is pressed
#define    KMETA_SHIFT   0x0800                                // Shift is pressed
#define    KMETA_ANY    (KMETA_ALT | KMETA_CTRL | KMETA_SHIFT)
#define    KMETA_CAPS    0x1000                                // CapsLock is on
#define    KMETA_NUM     0x2000                                // NumLock is on
#define    KMETA_SCRL    0x4000                                // ScrollLock is on

// Other keys
#define    KPRNT    ( KRT + 1 )
#define    KPAUSE   ( KPRNT + 1 )
#define    KLWIN    ( KPAUSE + 1 )
#define    KRWIN    ( KLWIN + 1 )
#define    KMENU    ( KRWIN + 1 )

#define    KRLEFT_CTRL        0x1D
#define    KRRIGHT_CTRL       0x1D

#define    KRLEFT_ALT         0x38
#define    KRRIGHT_ALT        0x38

#define    KRLEFT_SHIFT       0x2A
#define    KRRIGHT_SHIFT      0x36

#define    KRCAPS_LOCK        0x3A
#define    KRSCROLL_LOCK      0x46
#define    KRNUM_LOCK         0x45
#define    KRDEL              0x53

// Keymaps: US International

// Non-Shifted scancodes to ASCII:
static unsigned char ascii_non_shift[] = {
0, ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', BACKSPACE,
TAB, 'q', 'w',   'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',   '[', ']', ENTER, 0,
'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' ', 0,
KF1, KF2, KF3, KF4, KF5, KF6, KF7, KF8, KF9, KF10, 0, 0,
KHOME, KUP, KPGUP,'-', KLEFT, '5', KRIGHT, '+', KEND, KDOWN, KPGDN, KINS, KDEL, 0, 0, 0, KF11, KF12 };

// Shifted scancodes to ASCII:
static unsigned char ascii_shift[] = {
0, ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', BACKSPACE,
TAB, 'Q', 'W',   'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',   '{', '}', ENTER, 0,
'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|',
'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' ', 0,
KF1,   KF2, KF3, KF4, KF5, KF6, KF7, KF8, KF9, KF10, 0, 0,
KHOME, KUP, KPGUP, '-', KLEFT, '5',   KRIGHT, '+', KEND, KDOWN, KPGDN, KINS, KDEL, 0, 0, 0, KF11, KF12 };

struct special_keys modifiers;
buffer_t *buffer;

IRQ_HANDLER(keyboard_handler, 0)
{
	uint8_t scancode = inb(0x60);
    // Key released? Check bit 7 (0x80) of scan code for this
    if (scancode & 0x80) {
        scancode &= 0x7F; // Key was released, compare only low seven bits
        if (scancode == KRLEFT_SHIFT || scancode == KRRIGHT_SHIFT) {
            modifiers.shift = 0;
        }
    } else if (scancode == KRLEFT_SHIFT || scancode == KRRIGHT_SHIFT) {
        modifiers.shift = 1;
    } else {
        uint8_t b = buffer->mutex;
        buffer->mutex = 0;
        if (modifiers.shift == 1) {
            buffer->add(buffer, (char*)&ascii_shift[scancode], 1);
        } else {
            buffer->add(buffer, (char*)&ascii_non_shift[scancode], 1);
        }
        buffer->mutex = b;
    }
}

static uint32_t kb_read(vfs_file_t *file, void *buf, uint32_t size)
{
    if (size == 0) {
        return 0;
    }

    uint32_t done = buffer->get(buffer, buf, size);
    while(done == 0) {
        done = buffer->get(buffer, buf, size);
    }
    return done;
}

static vfs_file_operations_t kb_file_ops = {
    .open = 0,
    .close = 0,
    .write = 0,
    .read = &kb_read
};

void init_keyboard()
{
    buffer = create_buffer(100);
    create_vfs_device("/dev/kb", &kb_file_ops, 0);
	set_interrupt_gate(33, keyboard_handler, 0x08, 0x8E);

	uint8_t status;
	for(uint32_t timeout = 100000; timeout > 0; timeout--) {
		status = inb(0x64);
		if(status & 0x01) {
			inb(0x60);
			// what i have to do in case of parity error or timeout? bits 6 and 7.
			// if(status & (1 << 6) || status & (1 << 7))
		}
		break;
	}
}
