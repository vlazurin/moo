#ifndef H_GPU_DEVICE
#define H_GPU_DEVICE

#include "pci.h"

struct gpu_device
{
    pci_device_t *pci_dev;
    uint32_t width;
    uint32_t height;
};

#endif
