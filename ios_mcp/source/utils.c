#include "utils.h"
#include "imports.h"
#include "fsa.h"

#define COPY_BUFFER_SIZE 1024

#define HW_RSTB 0x0d800194

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

int SMC_SetNotificationLED(uint8_t mask)
{
    return bspWrite("SMC", 0, "NotificationLED", 1, &mask);
}

int SMC_SetODDPower(int power)
{
    return bspWrite("SMC", 0, "ODDPower", 4, &power);
}