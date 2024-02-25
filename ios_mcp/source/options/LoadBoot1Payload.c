#include "LoadBoot1Payload.h"

#include "menu.h"
#include "gfx.h"
#include "imports.h"
#include "utils.h"
#include "fsa.h"

#include <string.h>

static int preparePrshHax(uint32_t boootInfoPtr)
{
    // RAM vars
    uint32_t ram_start_addr = 0x10000000;
    uint32_t ram_test_buf_size = 0x400;

    // PRSH vars
    uint32_t prsh_hdr_offset = ram_start_addr + ram_test_buf_size + 0x5654;
    uint32_t prsh_hdr_size = 0x1C;

    // boot_info vars
    uint32_t boot_info_name_addr = prsh_hdr_offset + prsh_hdr_size;
    uint32_t boot_info_name_size = 0x100;
    uint32_t boot_info_ptr_addr = boot_info_name_addr + boot_info_name_size;

    // Calculate PRSH checksum
    uint32_t checksum_old = 0;
    uint32_t word_counter = 0;

    while (word_counter < 0x20D) {
        checksum_old ^= *(uint32_t *) (prsh_hdr_offset + 0x04 + word_counter * 0x04);
        word_counter++;
    }

    // Change boot_info to point inside boot1 memory
    *(uint32_t *) boot_info_ptr_addr = boootInfoPtr;

    // Re-calculate PRSH checksum
    uint32_t checksum = 0;
    word_counter = 0;

    while (word_counter < 0x20D) {
        checksum ^= *(uint32_t *) (prsh_hdr_offset + 0x04 + word_counter * 0x04);
        word_counter++;
    }

    // Update checksum
    *(uint32_t *) prsh_hdr_offset = checksum;

    // Copy PRSH IV from IOS-MCP
    void *prsh_iv_buf = IOS_HeapAlloc(CROSS_PROCESS_HEAP_ID, 0x10);
    if (!prsh_iv_buf) {
        return -1;
    }

    memcpy(prsh_iv_buf, (void*) 0x050677C0, 0x10);

    // Encrypt PRSH
    int res = encryptPrsh(0x10000400, 0x7C00, prsh_iv_buf, 0x10);

    // Free PRSH IV buffer
    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, prsh_iv_buf);

    // Flush cache
    IOS_FlushDCache((void*) 0x10000400, 0x7C00);

    return res;
}

typedef struct {
    uint16_t version;
    uint16_t sector;
    uint16_t reserved[4];
    uint32_t crc;
} Boot1Params;
static_assert(sizeof(Boot1Params) == 0x10);

Boot1Params boot1Params[2] __attribute__ ((aligned (0x10)));

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

void option_LoadBoot1Payload(void)
{
    static const Menu boot1ControlOptions[] = {
        {"Back", {0} },
        {"Load", {0} },
    };

    gfx_clear(COLOR_BACKGROUND);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_set_font_color(COLOR_PRIMARY);

    index = gfx_printf(16, index, GfxPrintFlag_ClearBG, "This will load a payload from within boot1\n"
                                                        "from the SD Card named boot1.img.\n\n"
                                                        "Do you want to continue?\n\n\n");

    int selected = drawMenu("Load BOOT1 Payload",
        boot1ControlOptions, ARRAY_SIZE(boot1ControlOptions), 0,
        MenuFlag_NoClearScreen, 16, index);
    index += (CHAR_SIZE_DRC_Y + 4) * (ARRAY_SIZE(boot1ControlOptions) + 1);

    if (selected == 0)
        return;

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
    uint32_t boootInfoPtr;
    switch (activeVersion) {
    case 8377:
        boootInfoPtr = 0x0D40AC6D;
        break;
    default:
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Unsupported BOOT1 version: %d", activeVersion);
        waitButtonInput();
        return;
    }

    int fileHandle;
    int res = FSA_OpenFile(fsaHandle, "/vol/storage_recovsd/boot1.img", "r", &fileHandle);
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

    res = preparePrshHax(boootInfoPtr);
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
    while(1);
}
