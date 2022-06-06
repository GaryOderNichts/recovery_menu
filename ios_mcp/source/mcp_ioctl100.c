#include "imports.h"
#include "menu.h"

static int threadStarted = 0;
static uint8_t threadStack[0x1000] __attribute__((aligned(0x20)));

int MCP_ioctl100_patch(void* msg)
{
    printf("MCP_ioctl100_patch %p\n", msg);

    // start the menu thread
    if (!threadStarted) {
        int tid = IOS_CreateThread(menuThread, NULL, threadStack + sizeof(threadStack), sizeof(threadStack), IOS_GetThreadPriority(0), 1);
        if (tid > 0) {
            IOS_StartThread(tid);
        }

        threadStarted = 1;
    }

    return 29;
}
