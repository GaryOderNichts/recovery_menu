#pragma once
#include <stdint.h>

#define SYSTEM_EVENT_FLAG_WAKE1            0x01
#define SYSTEM_EVENT_FLAG_WAKE0            0x02
#define SYSTEM_EVENT_FLAG_BT_INTERRUPT     0x04
#define SYSTEM_EVENT_FLAG_TIMER_SIGNAL     0x08
#define SYSTEM_EVENT_FLAG_DISC_INSERT      0x10
#define SYSTEM_EVENT_FLAG_EJECT_BUTTON     0x20
#define SYSTEM_EVENT_FLAG_POWER_BUTTON     0x40

uint32_t kernRead32(uint32_t address);

void kernWrite32(uint32_t address, uint32_t value);

int readOTP(void* buf, uint32_t size);

int EEPROM_Read(uint16_t offset, uint16_t num, uint16_t* buf);

int resetPPC(void);

int readSystemEventFlag(uint8_t* flag);

int copy_file(int fsaFd, const char* src, const char* dst);
