#include "vesa_lfb.h"
#include "log.h"
#include "shm.h"
#include "liballoc.h"
#include "string.h"
#include "gpu_device.h"
#include "port.h"
#include <stdbool.h>
#include "system.h"
#include "mm.h"

extern kernel_load_info_t *kernel_params;
/*
#define VBE_DISPI_IOPORT_INDEX          0x01CE
#define VBE_DISPI_IOPORT_DATA           0x01CF
#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9
#define VBE_DISPI_ID4 0xB0C4
#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_GETCAPS               0x02
#define VBE_DISPI_8BIT_DAC              0x20
#define VBE_DISPI_LFB_ENABLED           0x40
#define VBE_DISPI_NOCLEARMEM            0x80

#define VBE_DISPI_BPP_4                 0x04
#define VBE_DISPI_BPP_8                 0x08
#define VBE_DISPI_BPP_15                0x0F
#define VBE_DISPI_BPP_16                0x10
#define VBE_DISPI_BPP_24                0x18
#define VBE_DISPI_BPP_32                0x20

void BgaWriteRegister(unsigned short IndexValue, unsigned short DataValue)
{
    outw(VBE_DISPI_IOPORT_INDEX, IndexValue);
    outw(VBE_DISPI_IOPORT_DATA, DataValue);
}

unsigned short BgaReadRegister(unsigned short IndexValue)
{
    outw(VBE_DISPI_IOPORT_INDEX, IndexValue);
    return inw(VBE_DISPI_IOPORT_DATA);
}

int BgaIsAvailable(void)
{
    return (BgaReadRegister(VBE_DISPI_INDEX_ID) == VBE_DISPI_ID4);
}

void BgaSetVideoMode(unsigned int Width, unsigned int Height, unsigned int BitDepth, int UseLinearFrameBuffer, int ClearVideoMemory)
{
    BgaWriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    BgaWriteRegister(VBE_DISPI_INDEX_XRES, Width);
    BgaWriteRegister(VBE_DISPI_INDEX_YRES, Height);
    BgaWriteRegister(VBE_DISPI_INDEX_BPP, BitDepth);
    BgaWriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED |
        (UseLinearFrameBuffer ? VBE_DISPI_LFB_ENABLED : 0) |
        (ClearVideoMemory ? 0 : VBE_DISPI_NOCLEARMEM));
}

void BgaSetBank(unsigned short BankNumber)
{
    BgaWriteRegister(VBE_DISPI_INDEX_BANK, BankNumber);
}
*/
void init_vesa_lfb_gpu(pci_device_t *pci_device)
{
    struct gpu_device *video_dev = kmalloc(sizeof(struct gpu_device));
    memset(video_dev, 0, sizeof(struct gpu_device));

    video_dev->width = kernel_params->video_settings.width;
    video_dev->height = kernel_params->video_settings.height;

    size_t size = PAGE_ALIGN(video_dev->width * video_dev->height * 4);
    mark_memory_region(kernel_params->video_settings.framebuffer, size, true);
    int count = size / 0x1000;
    uintptr_t *pages = kmalloc(sizeof(uintptr_t) * count);
    for (int i = 0; i < count; i++) {
        pages[i] = kernel_params->video_settings.framebuffer + i * 0x1000;
    }
    int errno = shm_alloc("system_fb", size, pages, true);
    if (errno != 0) {
        log(KERN_ERR, "init_vesa_lfb_gpu failed\n");
    }
    kfree(pages);
    pci_device->hardware_driver = video_dev;
}
