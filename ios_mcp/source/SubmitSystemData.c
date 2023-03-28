/*
 *   Copyright (C) 2022-2023 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "SubmitSystemData.h"

#include "menu.h"
#include "gfx.h"
#include "imports.h"
#include "mdinfo.h"
#include "netconf.h"
#include "netdb.h"
#include "socket.h"
#include "utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define SYSDATA_HOST_NAME "wiiu.gerbilsoft.com"

// POST data
struct post_data {
    char system_model[16];  // [0x000] seeprom[0xB8]
    char system_serial[16]; // [0x010] seeprom[0xAC] + seeprom[0xB0] - need to mask the last 3 digits
    uint8_t mfg_date[6];    // [0x020] seeprom[0xC4]
    uint8_t productArea;    // [0x026]
    uint8_t gameRegion;     // [0x027]
    uint32_t sec_level;     // [0x028] otp[0x080]
    uint16_t boardType;     // [0x02C] seeprom[0x21]
    uint16_t boardRevision; // [0x02E] seeprom[0x22]
    uint16_t bootSource;    // [0x030] seeprom[0x23]
    uint16_t ddr3Size;      // [0x032] seeprom[0x24]
    uint16_t ddr3Speed;     // [0x034] seeprom[0x25]
    uint16_t sataDevice;    // [0x036] seeprom[0x2C]
    uint16_t consoleType;   // [0x038] seeprom[0x2D]
    uint16_t reserved1;     // [0x03A]
    uint32_t bsp_rev;       // [0x03C] bspGetHardwareVersion();
    uint8_t reserved2[80];  // [0x040]

    // [0x090]
    struct {
        uint32_t mid_prv;   // [0x090] Manufacturer and product revision
        uint32_t blockSize; // [0x094] Block size
        uint64_t numBlocks; // [0x098] Number of blocks
        char name1[128];    // [0x0A0] Product name
    } mlc;

    // [0x120]
    uint8_t otp_sha256[32]; // [0x120] OTP SHA-256 hash (to prevent duplicates)
};  // size == 0x140 (320)

struct post_data_hashed {
    struct post_data data;
    uint8_t post_sha256[32];    // [0x140] SHA-256 hash of post_data, with adjustments
};  // size == 0x160 (352)

/**
 * Get data from the received HTTP response header.
 *
 * NOTE: This modifies the HTTP response header to add
 * NUL bytes in order to parse individual messages.
 *
 * @param in_buf    [in] HTTP response header
 * @param len       [in] Size of HTTP response header
 * @param out_resp  [out] Start of "200 OK" or other response code
 * @param out_body  [out] Message body, or NULL if not available
 * @return 0 on success; non-zero if the header is invalid.
 */
static int parse_http_response(char *buf, size_t len, const char **out_resp, const char **out_body)
{
    // Start of the buffer must be in the format: "HTTP/1.x 200 OK\r\n"
    // - Can be 1.1 or 1.0.
    // - Response code must be 3 digits.
    if (len < 64) {
        // Not enough data in the buffer to parse anything.
        return -1;
    }
    *out_resp = NULL;
    *out_body = NULL;
    char *const p_end = buf + len;

    // Check for an HTTP response code.
    // TODO: Get scanf() and strchr() from IOSU?
    if (memcmp(buf, "HTTP/1.", 7) != 0)
        return -1;
    if (buf[7] != '0' && buf[7] != '1')
        return -1;
    if (buf[ 8] != ' ' || buf[12] != ' ' ||
       (buf[ 9] < '0' && buf[ 9] > '9') ||
       (buf[10] < '0' && buf[10] > '9') ||
       (buf[11] < '0' && buf[11] > '9')) {
        return -2;
    }

    // Find the '\r' and NULL it out.
    char *p = &buf[13];
    for (; p < p_end; p++) {
        if (*p == '\r') {
            *p = '\0';
            *out_resp = &buf[9];
            p++;
            break;
        }
    }
    if (p == p_end)
        return -3;

    // Find the end of the response header and the start of the message body.
    // This is delimited with the string "\r\n\r\n".
    // NOTE: If 204, there is no message body, but the server shouldn't
    // respond with 204.
    // TODO: Check "Content-Length"?
    for (; p < (p_end - 3); p++) {
        if (*p == '\r') {
            // Check if this is "\r\n\r\n".
            if (!memcmp(p, "\r\n\r\n", 4)) {
                // Found the start of the message body.
                p += 4;
                if (p < p_end) {
                    *out_body = p;
                }
                break;
            }
        }
    }
    return 0;
}

void option_SubmitSystemData(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Submit System Data");

    uint32_t index = 16 + 8 + 2 + 8;

    // parse OTP/SEEPROM for system information
    // 0x000-0x3FF: OTP
    // 0x400-0x5FF: SEEPROM
    // 0x600-0x7FF: post_data_hashed
#define DATA_BUFFER_SIZE 0x800
    uint8_t* dataBuffer = IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, DATA_BUFFER_SIZE, 0x40);
    if (!dataBuffer) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(16, index, 0, "Out of memory!");
        waitButtonInput();
        return;
    }
    if (read_otp_seeprom(dataBuffer, index) != 0) {
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        return;
    }

    static const char desc[] =
        "This will submit statistical data to the developers of recovery_menu,\n"
        "which will help to determine various statistics about Wii U consoles,\n"
        "e.g. eMMC manufacturers. The submitted data may be publicly accessible\n"
        "but personally identifying information will be kept confidential.\n"
        "\n"
        "Information that will be submitted:\n"
        "* System model\n"
        "* System serial number (excluding the last 3 digits)\n"
        "* Manufacturing date\n"
        "* Region information\n"
        "* Security level (keyset), sataDevice, consoleType, BSP revision\n"
        "* boardType, boardRevision, bootSource, ddr3Size, ddr3Speed\n"
        "* MLC manufacturer, revision, name, and size\n"
        "* SHA-256 hash of OTP (to prevent duplicates)\n"
        "\n"
        "Do you want to submit your console's system data?\n";
    gfx_set_font_color(COLOR_PRIMARY);
    index = gfx_print(16, index, 0, desc);
    index += 4;

    static const Menu submitSystemDataOptions[] = {
        {"Cancel", {0} },
        {"Submit Data", {0} },
    };
    int selected = drawMenu("Submit System Data",
        submitSystemDataOptions, ARRAY_SIZE(submitSystemDataOptions), 0,
        MenuFlag_NoClearScreen, 16, index);
    if (selected == 0)
        return;
    index += (CHAR_SIZE_DRC_Y*(ARRAY_SIZE(submitSystemDataOptions)+1)) + 4;

    /** Get the data and submit it. **/
    uint8_t* const otp = dataBuffer;
    uint16_t* const seeprom = (uint16_t*)dataBuffer + 0x200;
    struct post_data_hashed* const pdh = (struct post_data_hashed*)dataBuffer + 0x600;
    struct post_data* const pd = &pdh->data;

    // Copy in the POST data.
    memset(pd, 0, sizeof(*pd));
    memcpy(pd->system_model, &seeprom[0xB8], sizeof(pd->system_model));
    memcpy(pd->mfg_date, &seeprom[0xC4], sizeof(pd->mfg_date));
    memcpy(&pd->sec_level, &otp[0x080], sizeof(pd->sec_level));
    pd->boardType = seeprom[0x21];
    pd->boardRevision = seeprom[0x22];
    pd->bootSource = seeprom[0x23];
    pd->ddr3Size = seeprom[0x24];
    pd->ddr3Speed = seeprom[0x25];
    pd->sataDevice = seeprom[0x2C];
    pd->consoleType = seeprom[0x2D];

    // BSP revision
    int res = bspGetHardwareVersion(&pd->bsp_rev);
    if (res != 0) {
        pd->bsp_rev = 0;
    }

    // System serial number
    // NOTE: Assuming code+serial doesn't exceed 15 chars (plus NULL).
    snprintf(pd->system_serial, sizeof(pd->system_serial), "%s%s",
        (const char*)&seeprom[0xAC], (const char*)&seeprom[0xB0]);
    // Mask the last 3 digits of the system serial number.
    for (unsigned int i = sizeof(pd->system_serial)-1; i > 3; i--) {
        if (pd->system_serial[i] <= 0x20) {
            pd->system_serial[i] = 0;
            continue;
        }
        // Found printable text.
        // Mask the last three digits.
        pd->system_serial[i-0] = '*';
        pd->system_serial[i-1] = '*';
        pd->system_serial[i-2] = '*';
        break;
    }

    // Region information
    int productArea_id, gameRegion;
    res = getRegionInfo(&productArea_id, &gameRegion);
    if (res == 0) {
        // Region info obtained. (If not obtained, these will be 0.)
        pd->productArea = (uint8_t)productArea_id;
        pd->gameRegion = (uint8_t)gameRegion;
    }

    // MLC information
    MDReadInfo();	// TODO: Check for errors?
    for (unsigned int i = 0; i < 2; i++) {
        // We want the first MLC device.
        MDBlkDrv* drv = MDGetBlkDrv(i);
        if (!drv->registered || drv->params.device_type != SAL_DEVICE_TYPE_MLC) {
            continue;
        }

        pd->mlc.mid_prv = drv->params.mid_prv;
        pd->mlc.numBlocks = drv->params.numBlocks;
        pd->mlc.blockSize = drv->params.blockSize;
        memcpy(pd->mlc.name1, drv->params.name1, sizeof(pd->mlc.name1));
        break;
    }    

    // Hash the OTP ROM
    // TODO: Check for errors.
    uint8_t hash_ctx[IOSC_HASH_CONTEXT_SIZE];
    res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), NULL, 0, IOSC_HASH_FLAGS_INIT, NULL, 0);
    if (res == 0) {
        res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), otp, 0x400, IOSC_HASH_FLAGS_FINALIZE, pd->otp_sha256, sizeof(pd->otp_sha256));
    }

    // Hash the post data.
    // TODO: Check for errors.
    res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), NULL, 0, IOSC_HASH_FLAGS_INIT, NULL, 0);
    if (res == 0) {
        res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), (uint8_t*)pd, sizeof(*pd), IOSC_HASH_FLAGS_UPDATE, NULL, 0);
        if (res == 0) {
            // Add some more stuff to the hash.
            // NOTE: Reusing the OTP buffer here.
            memcpy(otp, desc, 64);
            res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), otp, 64, IOSC_HASH_FLAGS_FINALIZE, pdh->post_sha256, sizeof(pdh->post_sha256));
        }
    }

    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Submit System Data");

    // Initialize the network.
    index = 16 + 8 + 2 + 8;
    index = initNetconf(index);
    if (index == 0) {
        // An error occurred while initializing netconf.
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        waitButtonInput();
        return;
    }

    int httpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (httpSocket < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "socket() failed: %x", httpSocket);
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        waitButtonInput();
        return;
    }

    // Look up the domain name.
    struct hostent* h = gethostbyname(SYSDATA_HOST_NAME);
    if (!h || !h->h_addr) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(16, index, 0, "gethostbyname() failed; is your DNS server working?");
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        waitButtonInput();
        return;
    }

    // IP address is stored in h->h_addr.
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 80;
    memcpy(&sockaddr.sin_addr.s_addr, h->h_addr, 4);

    res = connect(httpSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "connect() failed: %x", res);
        closesocket(httpSocket);
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        waitButtonInput();
        return;
    }

    static const char submitting_data[] = "Connected, submitting data...";
    static const int status_xpos = 16 + (CHAR_SIZE_DRC_X * sizeof(submitting_data));
    gfx_print(16, index, 0, submitting_data);

    // To reduce processing requirements here, we'll submit a simple HTTP/1.0 request
    // without encryption.
    static const char http_req[] = "POST /add-system.php HTTP/1.0\r\n"
        "Host: wiiu.gerbilsoft.com\r\n"
        "User-Agent: Wii U Recovery Menu/" VERSION_STRING "\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: 352\r\n\r\n";

    // Send the HTTP request.
    // TODO: Do we need a newline after the POST data?
    send(httpSocket, http_req, sizeof(http_req)-1, 0);
    send(httpSocket, pdh, sizeof(*pdh), 0);

    // Wait for a response.
    // NOTE: Reusing dataBuffer here.
    // TODO: Show an error message aside from the HTTP response code?
    bool ok = false;
    res = recv(httpSocket, dataBuffer, DATA_BUFFER_SIZE-1, 0);
    dataBuffer[DATA_BUFFER_SIZE-1] = 0;
    if (res <= 0) {
        // No data received...
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(status_xpos, index, 0, "No response received from the server.");
        index += CHAR_SIZE_DRC_Y;
    } else {
        // Received data. Parse the header.
        if (res < DATA_BUFFER_SIZE) {
            dataBuffer[res] = 0;
        }
        const char *resp = NULL, *body = NULL;
        res = parse_http_response((char*)dataBuffer, res, &resp, &body);
        if (res == 0 && resp) {
            // If the response code is 2xx, success.
            // Otherwise, something failed.
            ok = (resp[0] == '2');
            gfx_set_font_color(ok ? COLOR_SUCCESS : COLOR_ERROR);
            gfx_print(status_xpos, index, 0, resp);
            index += CHAR_SIZE_DRC_Y;
            if (body) {
                // Print the message body.
                // TODO: Handle newlines if present?
                // TODO: Limit number of characters?
                gfx_print(16, index, 0, body);
                index += CHAR_SIZE_DRC_Y;
            }
            index += CHAR_SIZE_DRC_Y;
        } else {
            gfx_set_font_color(COLOR_ERROR);
            gfx_print(status_xpos, index, 0, "Invalid response received from the server.");
        }
    }

    closesocket(httpSocket);
    gfx_set_font_color(COLOR_PRIMARY);
    if (ok) {
        gfx_print(16, index, 0, "System data submitted successfully.");
        index += CHAR_SIZE_DRC_Y;
        static const char link_prefix[] = "Check out the Wii U console database at:";
        gfx_print(16, index, 0, link_prefix);
        static const int xpos = 16 + CHAR_SIZE_DRC_X * sizeof(link_prefix);
        gfx_set_font_color(COLOR_LINK);
        gfx_print(xpos, index, GfxPrintFlag_Underline, "https://" SYSDATA_HOST_NAME "/");
    } else {
        index = gfx_print(16, index, 0,
            "Failed to submit system data.\n"
            "Please report a bug on the GitHub issue tracker:\n");
        gfx_set_font_color(COLOR_LINK);
        gfx_print(16, index, GfxPrintFlag_Underline, "https://github.com/GaryOderNichts/recovery_menu/issues");
    }
    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
    waitButtonInput();
}
