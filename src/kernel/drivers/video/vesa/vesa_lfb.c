#include "vesa_lfb.h"
#include "debug.h"
#include "liballoc.h"
#include "string.h"
#include "gui/surface.h"
#include "gpu_device.h"
#include "system.h"
#include "mm.h"

extern kernel_load_info_t *kernel_params;

void bltblt(gpu_device_t *video_device, surface_t* surface)
{
    if (video_device->width != surface->width || video_device->height != surface->height)
    {
        debug("[vesa lfb]surface size != framebuffer size\n");
        return;
    }

    memcpy(video_device->addr, surface->addr, surface->width * surface->height * 4);
}

void init_vesa_lfb_video_adapter(pci_device_t *pci_device)
{
    debug("[vesa lfb] video driver init\n");
    gpu_device_t *video_dev = kmalloc(sizeof(gpu_device_t));
    memset(video_dev, 0, sizeof(gpu_device_t));
    video_dev->width = kernel_params->video_settings.width;
    video_dev->height = kernel_params->video_settings.height;

    uint32_t pages = PAGE_ALIGN(video_dev->width * video_dev->height * 4) / 0x1000;
    video_dev->addr = (void*)alloc_hardware_space_chunk(pages);
    map_virtual_to_physical_range((uint32_t)video_dev->addr, (uint32_t)kernel_params->video_settings.framebuffer, pages);

    video_dev->bltblt = &bltblt;
    pci_device->hardware_driver = video_dev;
    debug("[vesa lfb] work in %ix%i mode, framebuffer virtual location %h\n", video_dev->width, video_dev->height, video_dev->addr);
}
