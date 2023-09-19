#pragma once

#include <stdint.h>
#include <assert.h>

enum {
    SAL_DEVICE_TYPE_MLC = 5,
    SAL_DEVICE_TYPE_SD_CARD = 6,
};

typedef struct __attribute__((packed)) SALDeviceParams {
    uint32_t usrptr;
    uint32_t mid_prv;
    uint32_t device_type;
    uint32_t unk[7];
    uint64_t numBlocks;
    uint32_t blockSize;
    uint32_t unk2[6];
    char name0[128];
    char name1[128];
    char name2[128];
    uint32_t functions[12];
} SALDeviceParams;

typedef struct MDBlkDrv {
    int32_t registered;
    int32_t unk[2];
    struct SALDeviceParams params;
    int sal_handle;
    int deviceId;
    uint8_t unk2[196];
} MDBlkDrv;
static_assert(sizeof(MDBlkDrv) == 724, "MDBlkDrv: wrong size");

enum {
    MMC_RSP_R1      = 0x00,
    MMC_RSP_R1B     = 0x01,
    MMC_RSP_R2      = 0x02,
    MMC_RSP_R3      = 0x03,
    MMC_RSP_R4      = 0x04,
    MMC_RSP_R5      = 0x05,
    MMC_RSP_R6      = 0x06,
    MMC_RSP_R7      = 0x07,
    MMC_RSP_NONE    = 0x08,
    MMC_RSP_R5B     = 0x09,

    MMC_FLAG_READ                = 0x20,

    MMC_FLAG_AUTO_CMD12_ENABLE   = 0x100,
    MMC_FLAG_CHECK_WRITE_PROTECT = 0x200,

#define MMC_BUSY_TIMEOUT(x) (((x) & 0x1f) << 0xa)
};

typedef struct MMCCommand {
    uint32_t opcode;
    uint32_t arg;
    uint32_t flags;
    void* data;
    uint32_t blocks;
    uint32_t blocksize;
    uint32_t timeout;
} MMCCommand;

typedef struct MMCCommandResult {
    uint32_t response[4];
    uint32_t command_idx;
    int32_t error;
    uint32_t unk0x18;
    uint32_t unk0x1c;
    uint32_t intr_status;
} MMCCommandResult;

int MDReadInfo(void);

MDBlkDrv* MDGetBlkDrv(int index);

int MDGetCID(int deviceId, uint32_t* cid);

int MDGetCSD(int deviceId, uint32_t* csd);

int MDBlkDrv_Lock(int fsaHandle, int index);

int MDBlkDrv_Unlock(int fsaHandle, int index);

int MMC_Command(int fsaHandle, int32_t deviceId, MMCCommand* cmd, MMCCommandResult* res);
