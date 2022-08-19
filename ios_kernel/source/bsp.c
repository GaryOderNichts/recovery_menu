#include "bsp.h"
#include "imports.h"

static void* allocIobuf()
{
    void* ptr = IOS_HeapAlloc(0xcaff, 0x260);

    memset(ptr, 0, 0x260);

    return ptr;
}

static void freeIobuf(void* ptr)
{
    IOS_HeapFree(0xcaff, ptr);
}

int bspWrite(const char* entity, uint32_t instance, const char* attribute, uint32_t size, const void* buffer)
{
    int handle = IOS_Open("/dev/bsp", 0);
    if (handle < 0) {
        return handle;
    }

    uint32_t* buf = (uint32_t*) allocIobuf();
    strncpy((char*) buf, entity, 0x20);
    buf[8] = instance;
    strncpy((char*) (buf + 9), attribute, 0x20);
    buf[17] = size;
    memcpy((char*) (buf + 18), buffer, size);

    int res = IOS_Ioctl(handle, 6, buf, 0x48 + size, NULL, 0);

    freeIobuf(buf);
    IOS_Close(handle);

    return res;
}
