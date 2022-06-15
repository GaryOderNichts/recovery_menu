#pragma once

#include <imports.h>

int SCISetParentalEnable(uint8_t enable);

int SCIGetParentalEnable(uint8_t* outEnable);

int SCIGetParentalPinCode(char* pin, uint32_t pin_size);

int SCIGetParentalCustomSecQuestion(char* buf, uint32_t buf_size);

int SCIGetParentalSecAnswer(char* buf, uint32_t buf_size);
