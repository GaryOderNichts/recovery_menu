#include "mdinfo.h"
#include "utils.h"

// Since physical and virtual addresses match for IOS-FS, we can use these without address translation
#define MDBLK_DRIVER_ADDRESS 0x11c39e78
#define MD_DEVICE_POINTERS_ADDRESS 0x10899308

// TODO turn this into a struct eventually
#define MD_DEVICE_CID_OFFSET 0x58
#define MD_DEVICE_CSD_OFFSET 0x68

static MDBlkDrv blkDrvs[2] = { 0 };
static uint32_t devicePointers[8] = { 0 };

int MDReadInfo(void)
{
    uint32_t* dst = (uint32_t*) &blkDrvs;
    for (uint32_t i = 0; i < sizeof(blkDrvs) / 4; i++) {
        // use the kernel to read from IOS-FS memory
        dst[i] = kernRead32(MDBLK_DRIVER_ADDRESS + (i * 4));
    }

    for (uint32_t i = 0; i < sizeof(devicePointers) / 4; i++) {
        // use the kernel to read from IOS-FS memory
        devicePointers[i] = kernRead32(MD_DEVICE_POINTERS_ADDRESS + (i * 4));
    }

    return 0;
}

MDBlkDrv* MDGetBlkDrv(int index)
{
    return &blkDrvs[index];
}

int MDGetCID(int deviceId, uint32_t* cid)
{
    int idx = deviceId - 0x42;
    if (idx >= 8 || !devicePointers[idx]) {
        return -1;
    }

    for (uint32_t i = 0; i < 4; i++) {
        // use the kernel to read from IOS-FS memory
        cid[i] = kernRead32(devicePointers[idx] + MD_DEVICE_CID_OFFSET + (i * 4));
    }

    return 0;
}

int MDGetCSD(int deviceId, uint32_t* csd)
{
    int idx = deviceId - 0x42;
    if (idx >= 8 || !devicePointers[idx]) {
        return -1;
    }

    for (uint32_t i = 0; i < 4; i++) {
        // use the kernel to read from IOS-FS memory
        csd[i] = kernRead32(devicePointers[idx] + MD_DEVICE_CSD_OFFSET + (i * 4));
    }

    return 0;
}
