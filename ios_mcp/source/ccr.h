#pragma once

#include <stdint.h>

int CCRCDCGetMacAddress(int handle, uint8_t destId, void* macAddress);

int CCRSysGetPincode(int handle, uint8_t* pin);
