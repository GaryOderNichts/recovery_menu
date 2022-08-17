#include "mcp_misc.h"
#include "imports.h"

#include <string.h>

static void* allocIoBuf(uint32_t size)
{
    void* ptr = IOS_HeapAlloc(0xcaff, size);
    memset(ptr, 0, size);
    return ptr;
}

static void freeIoBuf(void* ptr)
{
    IOS_HeapFree(0xcaff, ptr);
}

int MCP_GetSysProdSettings(int handle, MCPSysProdSettings *out_sysProdSettings)
{
    uint8_t *buf = allocIoBuf(sizeof(IOSVec_t) + sizeof(*out_sysProdSettings));

    IOSVec_t* vecs = (IOSVec_t*)buf;
    vecs[0].ptr = buf + sizeof(IOSVec_t);
    vecs[0].len = sizeof(*out_sysProdSettings);

    int res = IOS_Ioctlv(handle, 0x40, 0, 1, vecs);
    if (res >= 0) {
        memcpy(out_sysProdSettings, vecs[0].ptr, sizeof(*out_sysProdSettings));
    }

    freeIoBuf(buf);

    return res;
}
