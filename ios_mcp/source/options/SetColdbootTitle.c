#include "SetColdbootTitle.h"

#include "menu.h"
#include "gfx.h"
#include "imports.h"

extern uint64_t currentColdbootOS;
extern uint64_t currentColdbootTitle;

void option_SetColdbootTitle(void)
{
    static const Menu coldbootTitleOptions[] = {
        {"Back", {0} },
        {"Wii U Menu (JPN)", {.tid = 0x0005001010040000}},
        {"Wii U Menu (USA)", {.tid = 0x0005001010040100}},
        {"Wii U Menu (EUR)", {.tid = 0x0005001010040200}},

        // non-retail systems only
        {"System Config Tool", {.tid = 0x000500101F700500}},
        {"DEVMENU (pre-2.09)", {.tid = 0x000500101F7001FF}},
        {"Kiosk Menu        ", {.tid = 0x000500101FA81000}},
    };

    // Only print the non-retail options if the system is in debug mode.
    const int option_count = ((IOS_CheckDebugMode() == 0) ? 7 : 4);

    int rval;
    uint64_t newtid = 0;
    int selected = 0;

    gfx_clear(COLOR_BACKGROUND);
    while (1) {
        uint32_t index = 16 + 8 + 2 + 8;
        gfx_set_font_color(COLOR_PRIMARY);

        // draw current titles
        gfx_printf(16, index, GfxPrintFlag_ClearBG, "Current coldboot title:    %08lx-%08lx",
            (uint32_t)(currentColdbootTitle >> 32), (uint32_t)(currentColdbootTitle & 0xFFFFFFFFU));
        index += CHAR_SIZE_DRC_Y + 4;

        gfx_printf(16, index, GfxPrintFlag_ClearBG, "Current coldboot os:       %08lx-%08lx",
            (uint32_t)(currentColdbootOS >> 32), (uint32_t)(currentColdbootOS & 0xFFFFFFFFU));
        index += (CHAR_SIZE_DRC_Y + 4) * 2;

        selected = drawMenu("Set Coldboot Title",
            coldbootTitleOptions, option_count, selected,
            MenuFlag_ShowTID | MenuFlag_NoClearScreen, 16, index);
        index += (CHAR_SIZE_DRC_Y + 4) * option_count;

        newtid = coldbootTitleOptions[selected].tid;
        if (newtid == 0)
            return;

        // set the new default title ID
        rval = setDefaultTitleId(newtid);

        if (newtid) {
            index += (CHAR_SIZE_DRC_Y + 4) * 2;

            gfx_set_font_color(COLOR_PRIMARY);
            gfx_printf(16, index, GfxPrintFlag_ClearBG,
                "Setting coldboot title id to %08lx-%08lx, rval %d  ",
                (uint32_t)(newtid >> 32), (uint32_t)(newtid & 0xFFFFFFFFU), rval);
            index += CHAR_SIZE_DRC_Y + 4;

            if (rval < 0) {
                gfx_set_font_color(COLOR_ERROR);
                gfx_print(16, index, GfxPrintFlag_ClearBG, "Error! Make sure title is installed correctly.");
            } else {
                gfx_set_font_color(COLOR_SUCCESS);
                gfx_print(16, index, GfxPrintFlag_ClearBG, "Success!                                      ");
            }
        }
    }
}
