#include "DumpSyslogs.h"

#include "menu.h"
#include "gfx.h"
#include "fsa.h"
#include "utils.h"

void option_DumpSyslogs(void)
{
    gfx_clear(COLOR_BACKGROUND);

    drawTopBar("Dumping Syslogs...");
    setNotificationLED(NOTIF_LED_RED_BLINKING, 0);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Creating 'logs' directory...");
    index += CHAR_SIZE_DRC_Y + 4;

    int res = FSA_MakeDir(fsaHandle, "/vol/storage_recovsd/logs", 0x600);
    if ((res < 0) && !(res == -0x30016)) {
        printf_error(index, "Failed to create directory: %x", res);
        return;
    }

    gfx_print(16, index, 0, "Opening system 'logs' directory...");
    index += CHAR_SIZE_DRC_Y + 4;

    int dir_handle;
    res = FSA_OpenDir(fsaHandle, "/vol/system/logs", &dir_handle);
    if (res < 0) {
        printf_error(index, "Failed to open system logs: %x", res);
        return;
    }

    char src_path[500];
    char dst_path[500];
    FSDirectoryEntry dir_entry;
    while (FSA_ReadDir(fsaHandle, dir_handle, &dir_entry) >= 0) {
        if (dir_entry.stat.flags & DIR_ENTRY_IS_DIRECTORY) {
            continue;
        }

        gfx_printf(16, index, GfxPrintFlag_ClearBG, "Copying %s...", dir_entry.name);

        snprintf(src_path, sizeof(src_path), "/vol/system/logs/" "%s", dir_entry.name);
        snprintf(dst_path, sizeof(dst_path), "/vol/storage_recovsd/logs/" "%s", dir_entry.name);

        res = copy_file(fsaHandle, src_path, dst_path);
        if (res < 0) {
            FSA_CloseDir(fsaHandle, dir_handle);
            index += CHAR_SIZE_DRC_Y + 4;
            printf_error(index, "Failed to copy %s: %x", dir_entry.name, res);
            return;
        }
    }

    setNotificationLED(NOTIF_LED_PURPLE, 0);
    index += CHAR_SIZE_DRC_Y + 4;
    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Done!");
    waitButtonInput();

    FSA_CloseDir(fsaHandle, dir_handle);
}
