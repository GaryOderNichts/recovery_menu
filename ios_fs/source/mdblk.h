#pragma once

#include "imports.h"

typedef struct SALDeviceParams SALDeviceParams;
typedef struct MDBlkDrv MDBlkDrv;
typedef struct MdBlkSetLockRequest MdBlkSetLockRequest;

struct PACKED SALDeviceParams {
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
};

struct MDBlkDrv {
    int32_t registered;
    int32_t unk[2];
    SALDeviceParams params;
    int sal_handle;
    int deviceId;
    uint8_t registerParams[28];
    uint32_t unk0x22c;
    int semaphore;
    uint8_t mtqHandler[0x98];
    uint32_t mtqMessages[2];
};
CHECK_SIZE(MDBlkDrv, 0x2d4);

struct MdBlkSetLockRequest {
    int32_t instance;
    int32_t lock;
};

extern MDBlkDrv mdBlkDrivers[2];

int FSA_Ioctl_MdBlk_SetLock(IPCRequest_t* request, void* inBuf, void* outBuf);
