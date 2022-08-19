#include "ccr.h"
#include "imports.h"
#include "utils.h"

#include <string.h>

static int cdc_handle = -1;

static void* allocIobuf()
{
    void* ptr = IOS_HeapAlloc(CROSS_PROCESS_HEAP_ID, 0x3a4);
    if (ptr) {
        memset(ptr, 0, 0x3a4);
    }

    return ptr;
}

static void freeIobuf(void* ptr)
{
    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, ptr);
}

static int makeRequest(uint32_t request_size, CCRDestinationID destId, int request, void* in, uint32_t in_size, void* out, uint32_t out_size)
{
    if (destId < CCR_DEST_DRH || destId > CCR_DEST_DRC1) {
        return -0x11fff9;
    }

    uint8_t* buf = allocIobuf();
    if (!buf) {
        return -0x11fffe;
    }

    // nsysccr writes this as a 4-byte int but IOSU only checks 1 byte
    buf[0x17] = destId;

    if (in && in_size) {
        memcpy(buf + 0x18, in, in_size);
    }

    IOSVec_t* vecs = (IOSVec_t*) (buf + ALIGN(request_size + 0x40, 0x40));
    vecs[0].ptr = buf;
    vecs[0].len = request_size;

    int result;        
    if (out && out_size) {
        vecs[1].ptr = buf;
        vecs[1].len = request_size;

        result = IOS_Ioctlv(cdc_handle, request, 1, 1, vecs);
        if (result == 0) {
            memcpy(out, buf + 0x18, out_size);
        }
    } else {
        result = IOS_Ioctlv(cdc_handle, request, 1, 0, vecs);
    }

    freeIobuf(buf);
    return result;
}

#define service0_makeRequest(...) makeRequest(0x38, __VA_ARGS__)
#define service1_makeRequest(...) makeRequest(0xc8, __VA_ARGS__)
#define service2_makeRequest(...) makeRequest(0x128, __VA_ARGS__)
#define service3_makeRequest(...) makeRequest(0x1d8, __VA_ARGS__)
#define service4_makeRequest(...) makeRequest(0x118, __VA_ARGS__)
#define service5_makeRequest(...) makeRequest(0x31c, __VA_ARGS__)

int CCRCDCSetup(void)
{
    if (cdc_handle > 0) {
        return 0;
    }

    int res = IOS_Open("/dev/ccr_cdc", 0);
    if (res > 0) {
        cdc_handle = res;
    }

    return 0;
}

int CCRCDCDevicePing(CCRDestinationID destId)
{
    return service2_makeRequest(destId, 0xc8, NULL, 0x0, NULL, 0x0);
}

int CCRCDCGetMacAddress(CCRDestinationID destId, void* macAddress)
{
    return service2_makeRequest(destId, 0xc9, NULL, 0, macAddress, 7);
}

int CCRCDCWpsStartEx(uint8_t* args)
{
    return service2_makeRequest(CCR_DEST_DRH, 0xce, args, 0xc, NULL, 0x0);
}

int CCRCDCWpsStop(void)
{
    return service2_makeRequest(CCR_DEST_DRH, 0xcf, NULL, 0x0, NULL, 0x0);
}

int CCRCDCWpsStatus(uint32_t* status)
{
    return service2_makeRequest(CCR_DEST_DRH, 0xd0, NULL, 0x0, status, 0x4);
}

int CCRCDCSysDrcDisplayMessage(CCRDestinationID destId, uint16_t* args)
{
    return service4_makeRequest(destId, 0x1a3, args, 0x4, NULL, 0x0);
}

int CCRCDCPerSetLcdMute(CCRDestinationID destId, uint8_t* mute)
{
    return service5_makeRequest(destId, 0x1fc, mute, 0x1, 0x0, 0x0);
}

int CCRSysGetPincode(uint8_t* pin)
{
    uint8_t mac[7];
    int res = CCRCDCGetMacAddress(CCR_DEST_DRH, mac);
    if (res == 0) {
        pin[3] = mac[6] & 3;
        pin[2] = (mac[6] >> 2) & 3;
        pin[1] = (mac[6] >> 4) & 3;
        pin[0] = mac[6] >> 6;
        return 0;
    }

    return res;
}

int CCRSysStartPairing(CCRDestinationID pairDest, uint16_t timeout, uint8_t* pin)
{
    uint8_t args[12];
    args[0] = 1;
    args[1] = '0' + pin[0];
    args[2] = '0' + pin[1];
    args[3] = '0' + pin[2];
    args[4] = '0' + pin[3];
    args[5] = '5';
    args[6] = '6';
    args[7] = '7';
    args[8] = '8';
    args[9] = timeout >> 8;
    args[10] = timeout & 0xff;
    args[11] = pairDest;
    return CCRCDCWpsStartEx(args);
}
