#pragma once

#include <stdint.h>
#include <assert.h>

enum {
    SAL_DEVICE_TYPE_MLC = 5,
    SAL_DEVICE_TYPE_SD_CARD = 6,
};

typedef struct __attribute__((packed)) SALDeviceParams {
    uint32_t usrptr;
    uint32_t mid_prv;
    uint32_t device_type;
    uint32_t unk[7];
    uint64_t numBlocks;
    uint32_t blockSize;
    uint32_t unk2[6];
    char name0[128];
    char name1[128];
    char name2[128];
    uint32_t functions[12];
} SALDeviceParams;

typedef struct MDBlkDrv {
    int32_t registered;
    int32_t unk[2];
    struct SALDeviceParams params;
    int sal_handle;
    int deviceId;
    uint8_t unk2[196];
} MDBlkDrv;
static_assert(sizeof(MDBlkDrv) == 724, "MDBlkDrv: wrong size");

int MDReadInfo(void);

MDBlkDrv* MDGetBlkDrv(int index);

int MDGetCID(int deviceId, uint32_t* cid);

int MDGetCSD(int deviceId, uint32_t* csd);
