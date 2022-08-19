#pragma once

#include <stdint.h>

int bspWrite(const char* entity, uint32_t instance, const char* attribute, uint32_t size, const void* buffer);
