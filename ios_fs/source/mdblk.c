#include "mdblk.h"
#include "imports.h"

int FSA_Ioctl_MdBlk_SetLock(IPCRequest_t* request, void* inBuf, void* outBuf)
{
    MdBlkSetLockRequest* req = (MdBlkSetLockRequest*) inBuf;
    if (req->instance >= 2) {
        IOS_ResourceReply(request, -1);
        return -1;
    }

    MDBlkDrv* blkDrv = &mdBlkDrivers[req->instance];
    if (!blkDrv->registered) {
        IOS_ResourceReply(request, -2);
        return -2;
    }

    // Lock/Unlock the driver
    int res;
    if (req->lock) {
        res = IOS_WaitSemaphore(blkDrv->semaphore, 0);
    } else {
        res = IOS_SignalSemaphore(blkDrv->semaphore);
    }

    IOS_ResourceReply(request, res);
    return res;
}
