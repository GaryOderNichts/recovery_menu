#include "ccr.h"
#include "imports.h"

#include <string.h>

static void* allocIobuf(uint32_t size)
{
    void* ptr = IOS_HeapAlloc(0xcaff, size);

    memset(ptr, 0x00, size);

    return ptr;
}

static void freeIobuf(void* ptr)
{
    IOS_HeapFree(0xcaff, ptr);
}

int CCRCDCGetMacAddress(int handle, uint8_t destId, void* macAddress)
{
    uint8_t* buf = allocIobuf(0x128 + sizeof(IOSVec_t) * 2);
    IOSVec_t* vecs = (IOSVec_t*) (buf + 0x128);

    buf[0x17] = destId;
    vecs[0].ptr = buf;
    vecs[0].len = 0x128;
    vecs[1].ptr = buf;
    vecs[1].len = 0x128;

    int res = IOS_Ioctlv(handle, 0xc9, 1, 1, vecs);
    if (res >= 0) {
        memcpy(macAddress, buf + 0x18, 7);
    }

    freeIobuf(buf);

    return res;
}

int CCRSysGetPincode(int handle, uint8_t* pin)
{
    uint8_t mac[7];
    int res = CCRCDCGetMacAddress(handle, 1, mac);
    if (res >= 0) {
        pin[3] = mac[6] & 3;
        pin[2] = (mac[6] >> 2) & 3;
        pin[1] = (mac[6] >> 4) & 3;
        pin[0] = mac[6] >> 6;
        return 0;
    }

    return res;
}
