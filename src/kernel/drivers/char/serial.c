#include "vfs.h"
#include "mutex.h"
#include "port.h"
#include "debug.h"
#include "buffer.h"
#include "irq.h"

#define SERIAL_PORT_1 0x3F8
#define SERIAL_PORT_2 0x2F8
#define SERIAL_PORT_3 0x3E8
#define SERIAL_PORT_4 0x2E8

static mutex_t mutex = 0;
buffer_t *buffers[4];
uint32_t ports[4] = {SERIAL_PORT_1, SERIAL_PORT_2, SERIAL_PORT_3, SERIAL_PORT_4};

static void setup_serial(uint32_t port)
{
    outb(port + 1, 0x00);    // Disable all interrupts
    outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(port + 1, 0x00);    //                  (hi byte)
    outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(port + 4, 0x01);    // IRQs: fire when data is available only
    outb(port + 1, 0x01);  // Enable interrupts
}

static int serial_write(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    uint32_t index = (uint32_t)file->node->obj;
    uint32_t done = 0;

    while(done < size)
    {
        while ((inb(ports[index] + 5) & 0x20) == 0);
        outb(ports[index], *(char*)buf);
        done++;
        buf++;
    }

    return done;
}

static int serial_read(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    uint32_t index = (uint32_t)file->node->obj;

    if (size == 0)
    {
        return 0;
    }

    uint32_t done = buffers[index]->get(buffers[index], buf, size);
    while(done == 0)
    {
        done = buffers[index]->get(buffers[index], buf, size);
    }
    return done;
}

static int serial_open(vfs_file_t *file, uint32_t flags)
{
    uint32_t index = (uint32_t)file->node->obj;
    buffers[index]->clear(buffers[index]);
    return 0;
}

static void read_serial_intr(uint32_t index)
{
    // we can read only one char per call,
    // because system has no ideas how much data will come and system can go to infinity loop
    while ((inb(ports[index] + 5) & 0x01) == 0);
    char c = inb(ports[index]);
    uint8_t b = buffers[index]->mutex;
    buffers[index]->mutex = 0;
    buffers[index]->add(buffers[index], (void*)&c, 1);
    buffers[index]->mutex = b;
}

static void serial_1_3_handler(struct regs *r)
{
    uint32_t index = 2;
	if (inb(SERIAL_PORT_1 + 1) & 0x01)
    {
		index = 0;
	}

    read_serial_intr(index);
}

static void serial_2_4_handler(struct regs *r)
{
    uint32_t index = 3;
	if (inb(SERIAL_PORT_2 + 1) & 0x01)
    {
		index = 1;
	}

    read_serial_intr(index);
}

static vfs_file_operations_t serial_file_ops = {
    .open = &serial_open,
    .close = 0,
    .write = &serial_write,
    .read = &serial_read
};

void init_serial()
{
    set_irq_handler(36, serial_1_3_handler);
    set_irq_handler(35, serial_2_4_handler);

    setup_serial(SERIAL_PORT_1);
    buffers[0] = create_buffer(128);
    create_vfs_node("/dev/ttyS0", S_IFCHR, &serial_file_ops, (void*)0, 0);
    setup_serial(SERIAL_PORT_2);
    buffers[1] = create_buffer(128);
    create_vfs_node("/dev/ttyS1", S_IFCHR, &serial_file_ops, (void*)1, 0);
    setup_serial(SERIAL_PORT_3);
    buffers[2] = create_buffer(128);
    create_vfs_node("/dev/ttyS2", S_IFCHR, &serial_file_ops, (void*)2, 0);
    setup_serial(SERIAL_PORT_4);
    buffers[3] = create_buffer(128);
    create_vfs_node("/dev/ttyS3", S_IFCHR, &serial_file_ops, (void*)3, 0);
}
