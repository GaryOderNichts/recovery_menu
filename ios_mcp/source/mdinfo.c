#include "mdinfo.h"
#include "utils.h"
#include "imports.h"

#include <string.h>

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

typedef struct MdBlkSetLockRequest {
    int32_t instance;
    int32_t lock;
} MdBlkSetLockRequest;

int MDBlkDrv_Lock(int fsaHandle, int index)
{
    MdBlkSetLockRequest req;
    req.instance = index;
    req.lock = 1;
    return IOS_Ioctl(fsaHandle, 0x200, &req, sizeof(req), NULL, 0);
}

int MDBlkDrv_Unlock(int fsaHandle, int index)
{
    MdBlkSetLockRequest req;
    req.instance = index;
    req.lock = 0;
    return IOS_Ioctl(fsaHandle, 0x200, &req, sizeof(req), NULL, 0);
}

typedef struct MMCCommandRequest {
    int32_t deviceId;
    uint32_t opcode;
    uint32_t arg;
    uint32_t flags;
    uint32_t blocks;
    uint32_t blocksize;
    uint32_t timeout;
} MMCCommandRequest;

typedef struct MMCCommandResponse {
    void* request;
    MMCCommandResult result;
} MMCCommandResponse;

int MMC_Command(int fsaHandle, int32_t deviceId, MMCCommand* cmd, MMCCommandResult* res)
{
    uint8_t* iobuf = IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, sizeof(MMCCommandRequest) + sizeof(MMCCommandResponse) + sizeof(IOSVec_t)*3, 0x40);
    if (!iobuf) {
        return -123;
    }

    MMCCommandRequest* vec0 = (MMCCommandRequest*) iobuf;
    vec0->deviceId = deviceId;
    vec0->opcode = cmd->opcode;
    vec0->arg = cmd->arg;
    vec0->flags = cmd->flags;
    vec0->blocks = cmd->blocks;
    vec0->blocksize = cmd->blocksize;
    vec0->timeout = cmd->timeout;

    MMCCommandResponse* vec1 = (MMCCommandResponse*) (iobuf + sizeof(*vec0));
    memset(vec1, 0, sizeof(*vec1));

    IOSVec_t* vecs = (IOSVec_t*) (iobuf + sizeof(*vec0) + sizeof(*vec1));
    vecs[0].ptr = vec0;
    vecs[0].len = sizeof(*vec0);
    vecs[1].ptr = vec1;
    vecs[1].len = sizeof(*vec1);
    vecs[2].ptr = cmd->data;
    vecs[2].len = cmd->blocks * cmd->blocksize;

    int rval = IOS_Ioctlv(fsaHandle, 0x201, 1, 2, vecs);
    memcpy(res, vec1, sizeof(*res));
    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, iobuf);
    return rval;
}
