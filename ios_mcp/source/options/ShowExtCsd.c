#include "ShowExtCsd.h"
#include "imports.h"
#include "menu.h"
#include "gfx.h"
#include "mdinfo.h"

void option_ShowExtCsd(void)
{
    gfx_clear(COLOR_BACKGROUND);

    drawTopBar("Showing EXT_CSD");

    uint32_t index = 16 + 8 + 2 + 8;

    MDReadInfo();
    MDBlkDrv* blkDrv = NULL;
    int blkDrvIndex = -1;
    for (unsigned int i = 0; i < 2; i++) {
        // We want the first MLC device.
        MDBlkDrv* drv = MDGetBlkDrv(i);
        if (!drv->registered || drv->params.device_type != SAL_DEVICE_TYPE_MLC) {
            continue;
        }

        blkDrvIndex = i;
        blkDrv = drv;
        break;
    }

    if (!blkDrv) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to find MLC drv");
        waitButtonInput();
        return;
    }

    // Lock blkdrv so it won't interfere with our custom commands
    MDBlkDrv_Lock(fsaHandle, blkDrvIndex);

    uint8_t* ext_csd = (uint8_t*) IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, 0x200, 0x20);
    if (!ext_csd) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to allocate ext_csd");
        waitButtonInput();
        return;
    }
    gfx_print(16, index, 0, "Submitting command...");
    index += CHAR_SIZE_DRC_Y + 4;

    MMCCommand cmd;
    cmd.opcode = 8;
    cmd.arg = 0;
    cmd.flags = MMC_FLAG_READ | MMC_RSP_R1;

    cmd.data = ext_csd;
    cmd.blocks = 1;
    cmd.blocksize = 0x200;
    cmd.timeout = 0x7d0;

    MMCCommandResult result;
    int res = MMC_Command(fsaHandle, blkDrv->deviceId, &cmd, &result);
    MDBlkDrv_Unlock(fsaHandle, blkDrvIndex);

    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to execute command: %x", res);
        waitButtonInput();
        return;
    }

    uint8_t ext_csd_rev = ext_csd[192];
    const char* str;
	switch (ext_csd_rev) {
	case 8:
		str = "5.1";
		break;
	case 7:
		str = "5.0";
		break;
	case 6:
		str = "4.5";
		break;
	case 5:
		str = "4.41";
		break;
	case 3:
		str = "4.3";
		break;
	case 2:
		str = "4.2";
		break;
	case 1:
		str = "4.1";
		break;
	case 0:
		str = "4.0";
		break;
	default:
		str = "Invalid revision";
        break;
	}

    gfx_printf(16, index, 0, "Extended CSD rev 1.%d (MMC %s)", ext_csd_rev, str);
    index += CHAR_SIZE_DRC_Y + 4;

    gfx_printf(16, index, 0, "S_CMD_SET: 0x%02x", ext_csd[504]);
    index += CHAR_SIZE_DRC_Y + 4;

	gfx_printf(16, index, 0, "BKOPS_SUPPORT: 0x%02x", ext_csd[502]);
    index += CHAR_SIZE_DRC_Y + 4;

	if (ext_csd_rev >= 5) {
		gfx_printf(16, index, 0, "BKOPS_EN: 0x%02x", ext_csd[163]);
        index += CHAR_SIZE_DRC_Y + 4;

		gfx_printf(16, index, 0, "BKOPS_STATUS: 0x%02x", ext_csd[246]);
        index += CHAR_SIZE_DRC_Y + 4;
    }

    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, ext_csd);

    waitButtonInput();
}
