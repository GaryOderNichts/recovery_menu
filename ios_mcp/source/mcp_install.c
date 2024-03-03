#include "mcp_install.h"
#include "imports.h"

#include <string.h>

static void* allocIoBuf(uint32_t size)
{
    void* ptr = IOS_HeapAlloc(CROSS_PROCESS_HEAP_ID, size);
    memset(ptr, 0, size);
    return ptr;
}

static void freeIoBuf(void* ptr)
{
    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, ptr);
}

int MCP_InstallGetInfo(int handle, const char* path, MCPInstallInfo* out_info)
{
    uint8_t* buf = allocIoBuf(0x27f + 0x16 + (sizeof(IOSVec_t) * 2));

    char* path_buf = (char*) buf + (sizeof(IOSVec_t) * 2);
    strncpy(path_buf, path, 0x27f);

    void* out_buf = buf + (sizeof(IOSVec_t) * 2) + 0x27f;

    IOSVec_t* vecs = (IOSVec_t*) buf;
    vecs[0].ptr = path_buf;
    vecs[0].len = 0x27f;
    vecs[1].ptr = out_buf;
    vecs[1].len = 0x16;

    int res = IOS_Ioctlv(handle, 0x80, 1, 1, vecs);
    if (res >= 0) {
        memcpy(out_info, out_buf, 0x16);
    }

    freeIoBuf(buf);

    return res;
}

int MCP_InstallSetTargetUsb(int handle, int target)
{
    uint32_t* buf = allocIoBuf(4);
    memcpy(buf, &target, 4);

    int res = IOS_Ioctl(handle, 0xf1, buf, 4, NULL, 0);

    freeIoBuf(buf);

    return res;
}

int MCP_InstallSetTargetDevice(int handle, int device)
{
    uint32_t* buf = allocIoBuf(4);
    memcpy(buf, &device, 4);

    int res = IOS_Ioctl(handle, 0x8d, buf, 4, NULL, 0);

    freeIoBuf(buf);

    return res;
}

int MCP_InstallTitle(int handle, const char* path)
{
    uint8_t* buf = allocIoBuf(0x27f + sizeof(IOSVec_t));

    char* path_buf = (char*) buf + sizeof(IOSVec_t);
    strncpy(path_buf, path, 0x27f);

    IOSVec_t* vecs = (IOSVec_t*) buf;
    vecs[0].ptr = path_buf;
    vecs[0].len = 0x27f;

    int res = IOS_Ioctlv(handle, 0x81, 1, 0, vecs);

    freeIoBuf(buf);

    return res;
}

int MCP_InstallTitleAsync(int handle, const char* path, int callbackQueue)
{
    uint8_t* buf = allocIoBuf(0x27f + sizeof(IOSVec_t) + sizeof(MCPAsyncReply));

    char* path_buf = (char*) buf + sizeof(IOSVec_t) + sizeof(MCPAsyncReply);
    strncpy(path_buf, path, 0x27f);

    IOSVec_t* vecs = (IOSVec_t*) buf;
    vecs[0].ptr = path_buf;
    vecs[0].len = 0x27f;

    MCPAsyncReply* reply = (MCPAsyncReply*) (buf + sizeof(IOSVec_t));
    reply->ioBuf = buf;

    int res = IOS_IoctlvAsync(handle, 0x81, 1, 0, vecs, callbackQueue, &reply->reply);
    if (res < 0) {
        freeIoBuf(buf);
    }

    return res;
}

int MCP_InstallGetProgress(int handle, MCPInstallProgress* progress)
{
    uint32_t* buf = allocIoBuf(0x24);

    int res = IOS_Ioctl(handle, 0x82, NULL, 0, buf, 0x24);
    if (res >= 0) {
        memcpy(progress, buf, 0x24);
    }

    freeIoBuf(buf);

    return res;
}
