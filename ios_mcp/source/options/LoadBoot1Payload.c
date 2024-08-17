#include "LoadBoot1Payload.h"

#include "menu.h"
#include "gfx.h"
#include "imports.h"
#include "utils.h"
#include "fsa.h"

#include <string.h>
#include <unistd.h>

typedef struct {
    uint16_t version;
    uint16_t sector;
    uint16_t reserved[4];
    uint32_t crc;
} Boot1Params;
static_assert(sizeof(Boot1Params) == 0x10);

Boot1Params boot1Params[2] __attribute__ ((aligned (0x10)));

static int preparePrshHax(uint32_t bootInfoOffset)
{
    // This code was taken from hexkyz' hexFW:
    // <https://github.com/hexkyz/hexFW/blob/19ee8f531030f0172401c31356287a667152b20c/firmware/programs/hexcore/source/main.c#L573-L624>

    // RAM vars
    const uint32_t ramStartAddr = 0x10000000;
    const uint32_t ramTestBufSize = 0x400;

    // PRSH vars
    const uint32_t prshHdrOffset = ramStartAddr + ramTestBufSize + 0x5654;
    const uint32_t prshHdrSize = 0x1c;

    // boot_info vars
    const uint32_t bootInfoNameAddr = prshHdrOffset + prshHdrSize;
    const uint32_t bootInfoNameSize = 0x100;
    const uint32_t bootInfoPtrAddr = bootInfoNameAddr + bootInfoNameSize;

    // Calculate PRSH checksum
    uint32_t checksumOld = 0;
    uint32_t wordCounter = 0;

    while (wordCounter < 0x20d) {
        checksumOld ^= *(uint32_t *) (prshHdrOffset + 0x04 + wordCounter * 0x04);
        wordCounter++;
    }

    // Change boot_info to point inside boot1 memory
    *(uint32_t *) bootInfoPtrAddr = 0x0d400200 + bootInfoOffset;

    // Re-calculate PRSH checksum
    uint32_t checksum = 0;
    wordCounter = 0;

    while (wordCounter < 0x20d) {
        checksum ^= *(uint32_t *) (prshHdrOffset + 0x04 + wordCounter * 0x04);
        wordCounter++;
    }

    // Update checksum
    *(uint32_t *) prshHdrOffset = checksum;

    // Copy PRSH IV from IOS-MCP
    void *prshIvBuf = IOS_HeapAlloc(CROSS_PROCESS_HEAP_ID, 0x10);
    if (!prshIvBuf) {
        return -1;
    }

    memcpy(prshIvBuf, (void*) 0x050677c0, 0x10);

    // Encrypt PRSH
    int res = encryptPrsh(0x10000400, 0x7c00, prshIvBuf, 0x10);

    // Free PRSH IV buffer
    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, prshIvBuf);

    // Flush cache
    IOS_FlushDCache((void*) 0x10000400, 0x7c00);

    return res;
}

static int decryptBoot1Params(int index)
{
    uint8_t iv[0x10];
    memset(iv, 0, sizeof(iv));

    Boot1Params* params = &boot1Params[index];
    IOS_FlushDCache(params, sizeof(*params));
    return IOSC_Decrypt(0x7, iv, sizeof(iv), params, sizeof(*params), params, sizeof(*params));
}

static int readBoot1Params(void)
{
    int ret;

    ret = EEPROM_Read(0xe8, 8, (uint16_t*)&boot1Params[0]);
    if (ret != 0) {
        return ret;
    }

    ret = EEPROM_Read(0xf0, 8, (uint16_t*)&boot1Params[1]);
    if (ret != 0) {
        return ret;
    }
        
    decryptBoot1Params(0);
    decryptBoot1Params(1);
    return 1;
}

static int determineActiveBoot1Slot(void)
{
    int activeSlot;

    // Calculate crcs
    uint32_t validMask = 0;
    for (int i = 0; i < 2; i++) {
        if (~crc32(0xffffffff, &boot1Params[i], 0xc) == boot1Params[i].crc) {
            validMask |= 1 << i;
        }
    }

    if (validMask == 0b01) {
        activeSlot = 0;
    } else if (validMask == 0b10) {
        activeSlot = 1;
    } else if (validMask == 0b11) {
        // Both versions are valid, check for newer version
        activeSlot = 0;
        if (boot1Params[1].version > boot1Params[0].version) {
            activeSlot = 1;
        }
    } else {
        activeSlot = -1;
    }

    return activeSlot;
}

static void loadBoot1Payload(uint32_t index, const char* filePath)
{
    gfx_printf(16, index, 0, "Checking BOOT1 version...");
    index += CHAR_SIZE_DRC_Y + 4;

    if (!readBoot1Params()) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read boot1 parameters");
        waitButtonInput();
        return;
    }

    int activeSlot;
    if ((activeSlot = determineActiveBoot1Slot()) < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to determine active boot1 version");
        waitButtonInput();
        return;
    }

    uint16_t activeVersion = boot1Params[activeSlot].version;
    gfx_printf(16, index, 0, "Active BOOT1 version: %d (slot %d)", activeVersion, activeSlot);
    index += CHAR_SIZE_DRC_Y + 4;

    // TODO support more versions / check for dev/retail
    uint32_t boootInfoOffset;
    switch (activeVersion) {
    case 8377:
        boootInfoOffset = 0xaa6d;
        break;
    default:
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Unsupported BOOT1 version: %d", activeVersion);
        waitButtonInput();
        return;
    }

    int fileHandle;
    int res = FSA_OpenFile(fsaHandle, filePath, "r", &fileHandle);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to open boot1.img: %x", res);
        waitButtonInput();
        return;
    }

    uint32_t* dataBuffer = (uint32_t*) IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, 0x200, 0x40);
    if (!dataBuffer) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to allocate data buffer!");
        waitButtonInput();
        FSA_CloseFile(fsaHandle, fileHandle);
        return;
    }

    // read the ancast header
    res = FSA_ReadFile(fsaHandle, dataBuffer, 1, 0x200, fileHandle, 0);
    if (res != 0x200) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read ancast header: %x", res);
        waitButtonInput();
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        FSA_CloseFile(fsaHandle, fileHandle);
        return;
    }

    // Check ancast magic
    if (dataBuffer[0] != 0xEFA282D9) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Invalid ancast header magic: %08lx", dataBuffer[0]);
        waitButtonInput();
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        FSA_CloseFile(fsaHandle, fileHandle);
        return;
    }    

    // Check unencrypted flag
    if (((dataBuffer[0x68] >> 16) & 1) == 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Ancast image is encrypted!");
        waitButtonInput();
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        FSA_CloseFile(fsaHandle, fileHandle);
        return;
    }

    uint32_t bodySize = dataBuffer[0x6B];

    // read and write payload to mem1
    uint32_t payloadOffset = 0x00000050;
    int bytesRead = 0;
    while ((res = FSA_ReadFile(fsaHandle, dataBuffer, 1, 0x40, fileHandle, 0)) > 0) {
        bytesRead += res;
        for (int i = 0; i < res; i += 4) {
            kernWrite32(payloadOffset, dataBuffer[i/4]);
            payloadOffset += 4;
        }
    }

    gfx_printf(16, index, 0, "Read %d / %ld bytes", bytesRead, bodySize);
    index += CHAR_SIZE_DRC_Y + 4;

    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
    FSA_CloseFile(fsaHandle, fileHandle);

    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read boot1.img: %x", res);
        waitButtonInput();
        return;
    }

    // Check body size
    if (bytesRead < bodySize) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read ancast body (%d / %ld bytes)", bytesRead, bodySize);
        waitButtonInput();
        return;
    }

    res = preparePrshHax(boootInfoOffset);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to prepare prshhax: %x", res);
        waitButtonInput();
        return;
    }

    // Setup branch to MEM1 payload
    kernWrite32(0x00000000, 0xEA000012); // b #0x50
    kernWrite32(0x00000004, 0xDEADC0DE);
    kernWrite32(0x00000008, 0xDEADC0DE);

    IOS_Shutdown(1);

    // we're at the point of no return
    while (1)
        ;
}

void option_LoadBoot1Payload(void)
{
    static const Menu boot1ControlOptions[] = {
        {"Back", {0} },
        {"Load", {0} },
    };

    gfx_clear(COLOR_BACKGROUND);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_set_font_color(COLOR_PRIMARY);

    index = gfx_printf(16, index, GfxPrintFlag_ClearBG, "This will load a payload from the SD Card named boot1.img\n"
                                                        "and execute it from within boot1.\n\n"
                                                        "Do you want to continue?\n\n");

    int selected = drawMenu("Load BOOT1 Payload",
        boot1ControlOptions, ARRAY_SIZE(boot1ControlOptions), 0,
        MenuFlag_NoClearScreen, 16, index);
    index += (CHAR_SIZE_DRC_Y + 4) * (ARRAY_SIZE(boot1ControlOptions) + 1);

    if (selected == 0)
        return;

    loadBoot1Payload(index, "/vol/storage_recovsd/boot1.img");
}

void handleBoot1Autoboot(void)
{
    static const char* autobootFile = "/vol/storage_recovsd/boot1now.img";

    // Check if the autoboot file exists
    FSStat stat;
    if (FSA_GetStat(fsaHandle, autobootFile, &stat) < 0) {
        return;
    }

    // Make sure it's not a directory
    if (stat.flags & DIR_ENTRY_IS_DIRECTORY) {
        return;
    }

    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Autobooting...");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_set_font_color(COLOR_PRIMARY);

    gfx_printf(16, index, 0, "Detected boot1now.img");
    index += CHAR_SIZE_DRC_Y + 4;

    uint64_t startTime;
    IOS_GetAbsTime64(&startTime);

    const int timeoutSecs = 5;
    int lastDraw = -1;
    uint8_t cur_flag = 0;
    uint8_t flag = 0;
    while (1) {
        SMC_ReadSystemEventFlag(&flag);
        if (cur_flag != flag) {
            if ((flag & SYSTEM_EVENT_FLAG_EJECT_BUTTON) || (flag & SYSTEM_EVENT_FLAG_POWER_BUTTON)) {
                return;
            }

            cur_flag = flag;
        }

        uint64_t curTime;
        IOS_GetAbsTime64(&curTime);
        uint32_t secondsPassed = (uint32_t) ((curTime - startTime) / 1000 / 1000);

        // Check if we reached the timeout
        if (secondsPassed > timeoutSecs) {
            break;
        }

        // Draw if seconds changed
        if (lastDraw != secondsPassed) {
            gfx_printf(16, index, GfxPrintFlag_ClearBG, "Autobooting boot1now.img in %ld seconds...\nPress any button to cancel", timeoutSecs - secondsPassed);
            lastDraw = secondsPassed;
        }
    }
    index += (CHAR_SIZE_DRC_Y + 4) * 2;

    loadBoot1Payload(index, autobootFile);
}
