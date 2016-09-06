#ifndef H_GPU_DEVICE
#define H_GPU_DEVICE

#include "pci.h"
#include "gui/surface.h"

typedef struct gpu_device gpu_device_t;

struct gpu_device
{
    pci_device_t *pci_dev;
    uint32_t width;
    uint32_t height;
    void *addr;
    void (*bltblt)(gpu_device_t*, surface_t*);
};

#endif
