#pragma once
#include <stdint.h>

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

#define SYSTEM_EVENT_FLAG_WAKE1            0x01
#define SYSTEM_EVENT_FLAG_WAKE0            0x02
#define SYSTEM_EVENT_FLAG_BT_INTERRUPT     0x04
#define SYSTEM_EVENT_FLAG_TIMER_SIGNAL     0x08
#define SYSTEM_EVENT_FLAG_DISC_INSERT      0x10
#define SYSTEM_EVENT_FLAG_EJECT_BUTTON     0x20
#define SYSTEM_EVENT_FLAG_POWER_BUTTON     0x40

enum {
    // 1280x720 XRGB framebuffer, outputs @720p
    DC_CONFIGURATION_0,
    // 1280x720 XRGB framebuffer, outputs @480p
    DC_CONFIGURATION_1,
};

typedef struct DisplayController_Config {
    uint32_t id;
    uint32_t field_0x4;
    int width;
    int height;
    void* framebuffer;
} DisplayController_Config;

uint32_t kernRead32(uint32_t address);

void kernWrite32(uint32_t address, uint32_t value);

int readOTP(void* buf, uint32_t size);

int EEPROM_Read(uint16_t offset, uint16_t num, uint16_t* buf);

int resetPPC(void);

int readSystemEventFlag(uint8_t* flag);

int copy_file(int fsaFd, const char* src, const char* dst);

int initDisplay(uint32_t configuration);

int readDCConfig(DisplayController_Config* config);
