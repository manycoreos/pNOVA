#ifndef __DM_LINEAR_H
#define __DM_LINEAR_H

#include "nova.h"
#include "super.h"

#define number_of_devices 2
typedef struct _nvmm_info
{
    void *virt_addr;
    const char *host;
    long size;
    struct dax_device *dax_dev;
    pfn_t __pfn_t;
}dm_nvmm_info;

dm_nvmm_info *multi_pm[number_of_devices];


#endif
