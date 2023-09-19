#include "mmc.h"
#include "imports.h"

#include <string.h>

static void MMCCommand_callback(void *arg, MMCCommandResult *result)
{
    MMCCommandResponse* resp = (MMCCommandResponse*) arg;
    if (resp) {
        memcpy(&resp->result, result, sizeof(*result));
        IOS_ResourceReply(resp->request, result->error);
    }
}

int FSA_Ioctlv_MMC_Command(IPCRequest_t* request, IOSVec_t* vecs)
{
    if (vecs[0].len != sizeof(MMCCommandRequest) || vecs[1].len != sizeof(MMCCommandResponse)) {
        IOS_ResourceReply(request, -1);
        return -1;
    }

    MMCCommandRequest* req = (MMCCommandRequest*) vecs[0].ptr;
    MMCCommandResponse* resp = (MMCCommandResponse*) vecs[1].ptr;

    MMCDevice* device = MMCGetDevice(req->deviceId);
    if (!device) {
        IOS_ResourceReply(request, -2);
        return -2;
    }

    MMCCommand command;
    MMCPrepareCommand(&command);
    command.opcode = req->opcode;
    command.arg = req->arg;
    command.flags = req->flags;

    MMCDmaOpts dma;
    uint32_t prevBlockSize = 0;
    if (req->blocks) {
        void* dma_buf = vecs[2].ptr;

        // dma buf needs to be aligned properly
        if ((uint32_t) dma_buf & 0x1f) {
            IOS_ResourceReply(request, -3);
            return -3;
        }

        // set block size (needs to be restored later)
        MMCPhysDevice* physDevice = AllocPool_Get(device->physDeviceId);
        prevBlockSize = physDevice->blockSize;
        physDevice->blockSize = req->blocksize;
        physDevice->blockSizeRegVal = req->blocksize | 0x7000;
        AllocPool_Ret(device->physDeviceId);

        dma.buf = dma_buf;
        dma.block_cnt = req->blocks;
        dma.timeout = req->timeout;
    }

    memset(resp, 0, sizeof(*resp));
    resp->request = request;
    int res = MMCPhysStartCommand(device->physDeviceId, &command, req->blocks ? &dma : NULL, MMCCommand_callback, resp);

    // restore block size
    if (prevBlockSize) {
        MMCPhysDevice* physDevice = AllocPool_Get(device->physDeviceId);
        physDevice->blockSize = prevBlockSize;
        physDevice->blockSizeRegVal = prevBlockSize | 0x7000;
        AllocPool_Ret(device->physDeviceId);
    }

    if (res != 0) {
        IOS_ResourceReply(request, res);
        return res;
    }

    return 0;
}
