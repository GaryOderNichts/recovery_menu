#pragma once
#include <stdint.h>
#include <assert.h>

/**
 * Number of elements in an array.
 *
 * Includes a static check for pointers to make sure
 * a dynamically-allocated array wasn't specified.
 * Reference: http://stackoverflow.com/questions/8018843/macro-definition-array-size
 */
#define ARRAY_SIZE(x) \
    (((sizeof(x) / sizeof(x[0]))) / \
        (size_t)(!(sizeof(x) % sizeof(x[0]))))

#define ALIGN(x, align) (((x) + ((align) -1)) & ~((align) -1))

enum {
    SYSTEM_EVENT_FLAG_WAKE1            = 1 << 0,
    SYSTEM_EVENT_FLAG_WAKE0            = 1 << 1,
    SYSTEM_EVENT_FLAG_BT_INTERRUPT     = 1 << 2,
    SYSTEM_EVENT_FLAG_TIMER_SIGNAL     = 1 << 3,
    SYSTEM_EVENT_FLAG_DISC_INSERT      = 1 << 4,
    SYSTEM_EVENT_FLAG_EJECT_BUTTON     = 1 << 5,
    SYSTEM_EVENT_FLAG_POWER_BUTTON     = 1 << 6,
};

enum {
    // 1280x720 XRGB framebuffer, outputs @720p
    DC_CONFIGURATION_0,
    // 1280x720 XRGB framebuffer, outputs @480p
    DC_CONFIGURATION_1,
};

typedef struct DC_Config {
    uint32_t id;
    uint32_t field_0x4;
    int width;
    int height;
    void* framebuffer;
} DC_Config;
static_assert(sizeof(DC_Config) == 0x14);

enum {
    NOTIF_LED_OFF               = 0,
    NOTIF_LED_ORANGE_BLINKING   = 1 << 0,
    NOTIF_LED_ORANGE            = 1 << 1,
    NOTIF_LED_RED_BLINKING      = 1 << 2,
    NOTIF_LED_RED               = 1 << 3,
    NOTIF_LED_BLUE_BLINKING     = 1 << 4,
    NOTIF_LED_BLUE              = 1 << 5,
    NOTIF_LED_PURPLE            = NOTIF_LED_RED | NOTIF_LED_BLUE,
};

uint32_t kernRead32(uint32_t address);

void kernWrite32(uint32_t address, uint32_t value);

int EEPROM_Read(uint16_t offset, uint16_t num, uint16_t* buf);

int resetPPC(void);

int copy_file(int fsaFd, const char* src, const char* dst);

int GFX_SubsystemInit(uint8_t unk);

int DISPLAY_DCInit(uint32_t configuration);

int DISPLAY_ReadDCConfig(DC_Config* config);

int SMC_ReadSystemEventFlag(uint8_t* flag);

int SMC_SetNotificationLED(uint8_t mask);

int SMC_SetODDPower(int power);
