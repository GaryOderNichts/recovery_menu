#pragma once

#include <stdint.h>

typedef enum {
    CCR_DEST_DRH = 1,
    CCR_DEST_DRC0,
    CCR_DEST_DRC1,
} CCRDestinationID;

int CCRCDCSetup(void);

int CCRCDCDevicePing(CCRDestinationID destId);

int CCRCDCGetMacAddress(CCRDestinationID destId, void* macAddress);

int CCRCDCWpsStartEx(uint8_t* args);

int CCRCDCWpsStop(void);

int CCRCDCWpsStatus(uint32_t* status);

int CCRCDCSysDrcDisplayMessage(CCRDestinationID destId, uint16_t* args);

int CCRCDCPerSetLcdMute(CCRDestinationID destId, uint8_t* mute);

int CCRSysGetPincode(uint8_t* pin);

int CCRSysStartPairing(CCRDestinationID pairDest, uint16_t timeout, uint8_t* pin);
