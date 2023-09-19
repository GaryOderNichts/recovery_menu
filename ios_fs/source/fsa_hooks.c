#include "imports.h"
#include "mmc.h"
#include "mdblk.h"

int __attribute__((used)) FSA_HandleIoctl_hook(IPCRequest_t* request, int command, void* inBuf, void* outBuf)
{
    if (command == 0x200) {
        return FSA_Ioctl_MdBlk_SetLock(request, inBuf, outBuf);
    }

    return FSA_HandleIoctl(request, command, inBuf, outBuf);
}

int __attribute__((used)) FSA_HandleIoctlv_hook(IPCRequest_t* request, int command, IOSVec_t* vecs)
{
    if (command == 0x201) {
        return FSA_Ioctlv_MMC_Command(request, vecs);
    }

    return FSA_HandleIoctlv(request, command, vecs);
}
