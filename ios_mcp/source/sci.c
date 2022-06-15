#include "sci.h"
#include <string.h>

static int _SCIReadSysConfig(const char* name, uint32_t type, uint32_t size, void* data)
{
    int ucHandle = UCOpen();
    if (ucHandle < 0) {
        return 0;
    }

    UCSysConfig_t conf;
    strncpy(conf.name, name, sizeof(conf.name));
    conf.access = 0x777;
    conf.error = 0;
    conf.data_type = type;
    conf.data_size = size;
    conf.data = data;
    int res = UCReadSysConfig(ucHandle, 1, &conf);

    IOS_Close(ucHandle);
    
    if (res == 0) {
        return 1;
    } else if (res == -0x200009) {
        return -4;
    } else if (res == -0x200015) {
        return -3;
    }

    return -1;
}

static int _SCIWriteSysConfig(const char* name, uint32_t type, uint32_t size, const void* data)
{
    int ucHandle = UCOpen();
    if (ucHandle < 0) {
        return 0;
    }

    UCSysConfig_t conf;
    strncpy(conf.name, name, sizeof(conf.name));
    conf.access = 0x777;
    conf.error = 0;
    conf.data_type = type;
    conf.data_size = size;
    conf.data = (void*) data;
    int res = UCWriteSysConfig(ucHandle, 1, &conf);

    IOS_Close(ucHandle);
    
    if (res == 0) {
        return 1;
    } else if (res == -0x200009) {
        return -4;
    } else if (res == -0x200015) {
        return -3;
    }

    return -1;
}

int SCISetParentalEnable(uint8_t enable)
{
    return _SCIWriteSysConfig("parent.enable", UC_DATA_TYPE_U8, 1, &enable);
}

int SCIGetParentalEnable(uint8_t* outEnable)
{
    return _SCIReadSysConfig("parent.enable", UC_DATA_TYPE_U8, 1, outEnable);
}

int SCIGetParentalPinCode(char* pin, uint32_t pin_size)
{
    return _SCIReadSysConfig("parent.pin_code", UC_DATA_TYPE_STRING, pin_size, pin);
}

int SCIGetParentalCustomSecQuestion(char* buf, uint32_t buf_size)
{
    return _SCIReadSysConfig("parent.custom_sec_question", UC_DATA_TYPE_STRING, buf_size, buf);
}

int SCIGetParentalSecAnswer(char* buf, uint32_t buf_size)
{
    return _SCIReadSysConfig("parent.sec_answer", UC_DATA_TYPE_STRING, buf_size, buf);
}
