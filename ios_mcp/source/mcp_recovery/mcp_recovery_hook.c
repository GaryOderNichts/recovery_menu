#include "imports.h"
#include "menu.h"

#include <string.h>

static int threadStarted = 0;
static uint8_t threadStack[0x1000] __attribute__((aligned(0x20)));

void* MCP_recovery_ioctl_memcpy_hook(void* dst, void* src, uint32_t size)
{
    printf("MCP_recovery_ioctl_memcpy_hook\n");

    // start the menu thread
    if (!threadStarted) {
        int tid = IOS_CreateThread(menuThread, NULL, threadStack + sizeof(threadStack), sizeof(threadStack), IOS_GetThreadPriority(0), 1);
        if (tid > 0) {
            IOS_StartThread(tid);
        }

        threadStarted = 1;
    }

    // perform the original copy
    return memcpy(dst, src, size);
}
