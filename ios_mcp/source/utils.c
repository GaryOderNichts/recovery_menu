#include "utils.h"
#include "imports.h"
#include "fsa.h"

#define COPY_BUFFER_SIZE 1024

#define HW_RSTB 0x0d800194

enum {
    ASYNC_MESSAGE_STOP_THREAD,
    ASYNC_MESSAGE_SET_LED,
};

static int asyncThreadHandle = -1;
static uint8_t asyncThreadStack[0x200] __attribute__((aligned(0x20)));
static uint32_t asyncMessageQueueBuf[0x20];
static int asyncMessageQueue = -1;
static int ledTimer = -1;
static uint8_t currentLedMask = NOTIF_LED_PURPLE;

static int SMC_SetNotificationLED(uint8_t mask);

static int asyncThread(void* arg)
{
    while (1) {
        // Blocking wait for messages
        uint32_t message;
        if (IOS_ReceiveMessage(asyncMessageQueue, &message, IOS_MESSAGE_FLAGS_NONE) < 0) {
            return 0;
        }

        switch (message) {
        case ASYNC_MESSAGE_STOP_THREAD:
            return 0;
        case ASYNC_MESSAGE_SET_LED:
            SMC_SetNotificationLED(currentLedMask);
            break;
        default:
            break;
        }
    }
}

int initializeUtils(void)
{
    asyncMessageQueue = IOS_CreateMessageQueue(asyncMessageQueueBuf, sizeof(asyncMessageQueueBuf) / 4);
    if (asyncMessageQueue < 0) {
        return -1;
    }

    asyncThreadHandle = IOS_CreateThread(asyncThread, NULL, asyncThreadStack + sizeof(asyncThreadStack), sizeof(asyncThreadStack), IOS_GetThreadPriority(0) - 0x20, IOS_THREAD_FLAGS_NONE);
    if (asyncThreadHandle < 0) {
        return -1;
    }

    if (IOS_StartThread(asyncThreadHandle) < 0) {
        return -1;
    }

    ledTimer = IOS_CreateTimer(0, 0, asyncMessageQueue, ASYNC_MESSAGE_SET_LED);
    if (ledTimer < 0) {
        return -1;
    }

    return 0;
}

int finalizeUtils(void)
{
    // Tell thread to stop and wait
    IOS_SendMessage(asyncMessageQueue, ASYNC_MESSAGE_STOP_THREAD, IOS_MESSAGE_FLAGS_NONE);
    IOS_JoinThread(asyncThreadHandle, NULL);

    IOS_DestroyTimer(ledTimer);
    IOS_DestroyMessageQueue(asyncMessageQueue);

    return 0;
}

uint32_t kernRead32(uint32_t address)
{
    return IOS_Syscall0x81(0, address, 0);
}

void kernWrite32(uint32_t address, uint32_t value)
{
    IOS_Syscall0x81(1, address, value);
}

int EEPROM_Read(uint16_t offset, uint16_t num, uint16_t* buf)
{
    if (offset + num > 0x100) {
        return -0x1d;
    }

    for (uint16_t i = offset; i < offset + num; i++) {
        uint16_t tmp;
        int res = bspRead("EE", i, "access", 2, &tmp);
        if (res < 0) {
            return res;
        }

        *buf = tmp;
        buf++;
    }

    return 0;
}

int resetPPC(void)
{
    // cannot reset ppc through bsp?
    // uint8_t val = 1;
    // return bspWrite("PPC", 0, "Exe", 1, &val);

    kernWrite32(HW_RSTB, kernRead32(HW_RSTB) & ~0x210);
    return 0;
}

int copy_file(int fsaFd, const char* src, const char* dst)
{
    int readHandle;
    int res = FSA_OpenFile(fsaFd, src, "r", &readHandle);
    if (res < 0) {
        return res;
    }

    int writeHandle;
    res = FSA_OpenFile(fsaFd, dst, "w", &writeHandle);
    if (res < 0) {
        FSA_CloseFile(fsaFd, readHandle);
        return res;
    }

    void* dataBuffer = IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, COPY_BUFFER_SIZE, 0x40);
    if (!dataBuffer) {
        FSA_CloseFile(fsaFd, readHandle);
        FSA_CloseFile(fsaFd, writeHandle);
        return -1;
    }

    while ((res = FSA_ReadFile(fsaFd, dataBuffer, 1, COPY_BUFFER_SIZE, readHandle, 0)) > 0) {
        if ((res = FSA_WriteFile(fsaFd, dataBuffer, 1, res, writeHandle, 0)) < 0) {
            break;
        }
    }

    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
    FSA_CloseFile(fsaFd, writeHandle);
    FSA_CloseFile(fsaFd, readHandle);

    return (res > 0) ? 0 : res;
}

int GFX_SubsystemInit(uint8_t unk)
{
    return bspInit("GFX", 0, "subsystem", 1, &unk);
}

int DISPLAY_DCInit(uint32_t configuration)
{
    return bspWrite("DISPLAY", 0, "DC_INIT", 4, &configuration);
}

int DISPLAY_ReadDCConfig(DC_Config* config)
{
    return bspRead("DISPLAY", 0, "DC_CONFIG", sizeof(DC_Config), config);
}

int SMC_ReadSystemEventFlag(uint8_t* flag)
{
    return bspRead("SMC", 0, "SystemEventFlag", 1, flag);
}

static int SMC_SetNotificationLED(uint8_t mask)
{
    return bspWrite("SMC", 0, "NotificationLED", 1, &mask);
}

int SMC_SetODDPower(int power)
{
    return bspWrite("SMC", 0, "ODDPower", 4, &power);
}

void setNotificationLED(uint8_t mask, uint32_t duration)
{
    // Cancel any pending led updates
    IOS_StopTimer(ledTimer);

    if (currentLedMask == mask) {
        return;
    }

    SMC_SetNotificationLED(mask);

    if (duration) {
        // Don't update the current LED mask and tell the async thread to restore the current LED mask
        // after the specified duration
        IOS_RestartTimer(ledTimer, duration * 1000, 0);
    } else {
        currentLedMask = mask;
    }
}
