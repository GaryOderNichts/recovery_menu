#include "InstallWUP.h"

#include "menu.h"
#include "gfx.h"
#include "mcp_install.h"
#include "imports.h"
#include "utils.h"
#include <unistd.h>

static int callbackQueue = -1;
static uint8_t callbackThreadStack[0x200] __attribute__((aligned(0x20)));
static int asyncPending = 0;
static int asyncResult = -1;

static int callbackThread(void* arg)
{
    MCPAsyncReply* reply = NULL;
    int ret = IOS_ReceiveMessage(callbackQueue, (uint32_t*)&reply, IOS_MESSAGE_FLAGS_NONE);
    if (ret >= 0 && reply) {
        asyncResult = reply->reply.result;
        asyncPending = 0;

        // Free original request
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, reply->ioBuf);
    }

    return 0;
}

void option_InstallWUP(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Installing WUP");
    setNotificationLED(NOTIF_LED_RED_BLINKING, 0);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Make sure to place a valid signed WUP directly in 'sd:/install'");
    index += CHAR_SIZE_DRC_Y + 4;

    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle < 0) {
        printf_error(index, "Failed to open /dev/mcp: %x", mcpHandle);
        return;
    }

    gfx_print(16, index, 0, "Querying install info...");
    index += CHAR_SIZE_DRC_Y + 4;

    MCPInstallInfo info;
    int res = MCP_InstallGetInfo(mcpHandle, "/vol/storage_recovsd/install", &info);
    if (res < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "Failed to get install info: %x", res);
        return;
    }

    gfx_printf(16, index, 0, "Installing title: %08lx-%08lx...",
        (uint32_t)(info.titleId >> 32), (uint32_t)(info.titleId & 0xFFFFFFFFU));
    index += CHAR_SIZE_DRC_Y + 4;

    // only install to NAND
    res = MCP_InstallSetTargetDevice(mcpHandle, 0);
    if (res < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "MCP_InstallSetTargetDevice: %x", res);
        return;
    }
    res = MCP_InstallSetTargetUsb(mcpHandle, 0);
    if (res < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "MCP_InstallSetTargetUsb: %x", res);
        return;
    }

    int callbackThreadId = IOS_CreateThread(callbackThread, NULL, callbackThreadStack + sizeof(callbackThreadStack), sizeof(callbackThreadStack), IOS_GetThreadPriority(0), IOS_THREAD_FLAGS_NONE);
    if (callbackThreadId < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "IOS_CreateThread: %x", res);
        return;   
    }

    if (IOS_StartThread(callbackThreadId) < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "IOS_StartThread failed");
        return; 
    }

    uint32_t messages[5];
    callbackQueue = IOS_CreateMessageQueue(messages, sizeof(messages) / sizeof(uint32_t));
    if (callbackQueue < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "IOS_CreateThread: %x", res);
        return;
    }

    asyncResult = -1;
    asyncPending = 1;
    res = MCP_InstallTitleAsync(mcpHandle, "/vol/storage_recovsd/install", callbackQueue);
    if (res < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "Failed to install: %x", res);
        return;
    }

    while (asyncPending) {
        MCPInstallProgress progress;
        res = MCP_InstallGetProgress(mcpHandle, &progress);
        if (res < 0) {
            // Uh oh
        }

        gfx_printf(16, index, GfxPrintFlag_ClearBG, "Installing (%ld)... (%lu KiB / %lu KiB)", progress.inProgress, (uint32_t) (progress.sizeProgress / 1024llu), (uint32_t) (progress.sizeTotal / 1024llu));
        usleep(50 * 1000);
    }
    index += CHAR_SIZE_DRC_Y + 4;

    setNotificationLED(NOTIF_LED_PURPLE, 0);
    gfx_set_font_color(COLOR_SUCCESS);
    gfx_printf(16, index, 0, "Done (with result %d)!", asyncResult);
    waitButtonInput();

    IOS_JoinThread(callbackThreadId, NULL);
    IOS_DestroyMessageQueue(callbackQueue);

    IOS_Close(mcpHandle);
}
