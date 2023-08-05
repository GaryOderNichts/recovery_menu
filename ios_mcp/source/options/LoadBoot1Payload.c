#include "LoadBoot1Payload.h"

#include "menu.h"
#include "gfx.h"
#include "imports.h"
#include "utils.h"
#include "fsa.h"

#include <string.h>

static int preparePrshHax(void)
{
    // RAM vars
    uint32_t ram_start_addr    = 0x10000000;
    uint32_t ram_test_buf_size = 0x400;

    // PRSH vars
    uint32_t prsh_hdr_offset = ram_start_addr + ram_test_buf_size + 0x5654;
    uint32_t prsh_hdr_size   = 0x1C;

    // boot_info vars
    uint32_t boot_info_name_addr = prsh_hdr_offset + prsh_hdr_size;
    uint32_t boot_info_name_size = 0x100;
    uint32_t boot_info_ptr_addr  = boot_info_name_addr + boot_info_name_size;

    // Calculate PRSH checksum
    uint32_t checksum_old = 0;
    uint32_t word_counter = 0;

    while (word_counter < 0x20D) {
        checksum_old ^= *(uint32_t *) (prsh_hdr_offset + 0x04 + word_counter * 0x04);
        word_counter++;
    }

    // Change boot_info to point inside boot1 memory
    *(uint32_t *) boot_info_ptr_addr = 0x0D40AC6D;

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

    int fileHandle;
    int res = FSA_OpenFile(fsaHandle, "/vol/storage_recovsd/boot1.img", "r", &fileHandle);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to open boot1.img: %x", res);
        waitButtonInput();
        return;
    }

    uint32_t* dataBuffer = (uint32_t*) IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, 0x40, 0x40);
    if (!dataBuffer) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to allocate data buffer!");
        waitButtonInput();
        FSA_CloseFile(fsaHandle, fileHandle);
        return;
    }

    // // skip the ancast header
    // res = FSA_SetPosFile(fsaHandle, fileHandle, 0x200);
    // if (res < 0) {
    //     gfx_set_font_color(COLOR_ERROR);
    //     gfx_printf(16, index, 0, "Failed to set boot1.img pos: %x", res);
    //     waitButtonInput();
    //     IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
    //     FSA_CloseFile(fsaHandle, fileHandle);
    //     return;
    // }

    // read and write payload to mem1
    uint32_t payloadOffset = 0x00000050;
    int bytesRead = 0;
    while ((res = FSA_ReadFile(fsaHandle, dataBuffer, 1, 0x40, fileHandle, 0)) > 0) {
        bytesRead += res;
        for (int i = 0; i < res; i += 4) {
            kernWrite32(payloadOffset, dataBuffer[i]);
            payloadOffset += 4;
        }
    }

    gfx_printf(16, index, 0, "Read %d bytes", bytesRead);
    waitButtonInput();

    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
    FSA_CloseFile(fsaHandle, fileHandle);

    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read boot1.img: %x", res);
        waitButtonInput();
        return;
    }

    res = preparePrshHax();
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to prepare prshhax: %x", res);
        waitButtonInput();
        return;
    }

    // Setup branch to MEM1 payload (skip ancast + loader header)
    kernWrite32(0x00000000, 0xEA000096); // b #0x260
    kernWrite32(0x00000004, 0xDEADC0DE);
    kernWrite32(0x00000008, 0xDEADC0DE);

    IOS_Shutdown(1);

    // we're at the point of no return
    while(1);
}
