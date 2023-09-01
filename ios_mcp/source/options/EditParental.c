#include "EditParental.h"

#include "menu.h"
#include "gfx.h"
#include "sci.h"
#include "utils.h"

void option_EditParental(void)
{
    static const Menu parentalControlOptions[] = {
        {"Back", {0} },
        {"Disable", {0} },
    };

    int rval;
    int selected = 0;

    gfx_clear(COLOR_BACKGROUND);
    while (1) {
        uint32_t index = 16 + 8 + 2 + 8;
        gfx_set_font_color(COLOR_PRIMARY);

        // draw current parental control info
        uint8_t enabled = 0;
        int res = SCIGetParentalEnable(&enabled);
        if (res == 1) {
            gfx_set_font_color(COLOR_PRIMARY);
            gfx_printf(16, index, GfxPrintFlag_ClearBG, "Parental Controls: %s",
                enabled ? "Enabled" : "Disabled");
        } else {
            gfx_set_font_color(COLOR_ERROR);
            gfx_printf(16, index, GfxPrintFlag_ClearBG, "SCIGetParentalEnable failed: %d", res);
        }
        index += CHAR_SIZE_DRC_Y + 4;

        char pin[5] = "";
        res = SCIGetParentalPinCode(pin, sizeof(pin));
        if (res == 1) {
            gfx_set_font_color(COLOR_PRIMARY);
            gfx_printf(16, index, GfxPrintFlag_ClearBG, "Parental Pin Code: %s", pin);
        } else {
            gfx_set_font_color(COLOR_ERROR);
            gfx_printf(16, index, GfxPrintFlag_ClearBG, "SCIGetParentalPinCode failed: %d", res);
        }
        index += (CHAR_SIZE_DRC_Y + 4) * 2;

        gfx_set_font_color(COLOR_PRIMARY);

        selected = drawMenu("Edit Parental Controls",
            parentalControlOptions, ARRAY_SIZE(parentalControlOptions), selected,
            MenuFlag_NoClearScreen, 16, index);
        index += (CHAR_SIZE_DRC_Y + 4) * (ARRAY_SIZE(parentalControlOptions) + 1);

        if (selected == 0)
            return;

        // Option 1: Disable the parental controls.
        rval = SCISetParentalEnable(0);

        gfx_printf(16, index, GfxPrintFlag_ClearBG, "SCISetParentalEnable(false): %d  ", rval);
        index += CHAR_SIZE_DRC_Y + 4;

        if (rval != 1) {
            gfx_set_font_color(COLOR_ERROR);
            gfx_print(16, index, GfxPrintFlag_ClearBG, "Error!  ");
        } else {
            gfx_set_font_color(COLOR_SUCCESS);
            gfx_print(16, index, GfxPrintFlag_ClearBG, "Success!");
        }
        index += CHAR_SIZE_DRC_Y + 4;
    }
}
