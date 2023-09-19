#pragma once

#include "imports.h"

typedef struct MMCDevice MMCDevice;
typedef struct MMCPhysDevice MMCPhysDevice;
typedef struct MMCCommand MMCCommand;
typedef struct MMCDmaOpts MMCDmaOpts;
typedef struct MMCCommandResult MMCCommandResult;
typedef struct MMCCommandRequest MMCCommandRequest;
typedef struct MMCCommandResponse MMCCommandResponse;

struct MMCDevice {
    MMCDevice *next;
    int32_t physDeviceId;
    uint32_t unk0x8;
    int32_t semaphore0;
    int32_t semaphore1;
    int32_t semaphore2;
    int32_t idx;
    uint32_t unk0x1c;
    uint32_t rca;
    uint32_t unk0x24;
    uint32_t xfer;
    uint32_t flags;
    uint32_t unk0x30;
    uint32_t nsac;
    uint32_t unk0x38;
    uint32_t unk0x3c;
    uint32_t read_timeout;
    uint32_t write_timeout;
    uint32_t unk0x48;
    uint32_t unk0x4c;
    uint32_t error_mask;
    uint32_t unk0x50;
    uint32_t cid[4];
    uint32_t csd[4];
    uint32_t unk0x78;
    void *unregisterCallback;
    void *unregisterArg;
    void *callback;
    uint32_t unk0x88;
    void *arg;
    uint32_t unk0x90;
    uint32_t access_time;
    uint32_t unk0x98;
    uint32_t unk0x9c;
    uint32_t blks;
    uint32_t lba;
    uint32_t unk0xa8;
};
CHECK_SIZE(MMCDevice, 0xAC);

struct MMCCommand {
    uint32_t opcode;
    uint32_t command_idx;
    uint32_t arg;
    uint32_t flags;
};
CHECK_SIZE(MMCCommand, 0x10);

struct MMCDmaOpts {
    void *buf;
    uint32_t block_cnt;
    uint32_t timeout;
};
CHECK_SIZE(MMCDmaOpts, 0xC);

struct MMCCommandResult {
    uint32_t response[4];
    uint32_t command_idx;
    int32_t error;
    uint32_t unk0x18;
    uint32_t unk0x1c;
    uint32_t intr_status;
};
CHECK_SIZE(MMCCommandResult, 0x24);

struct MMCPhysDevice {
    void *regBase;
    int32_t regOffset;
    uint32_t unk0x8;
    uint32_t unk0xC;
    int32_t semaphore;
    uint32_t *unk0x14;
    uint16_t unk0x18;
    uint32_t unk0x1C;
    uint32_t unk0x20;
    uint32_t blockSize;
    uint32_t unk0x28;
    MMCCommand curCmd;
    MMCDmaOpts curDma;
    MMCCommandResult curRes;
    void *commandCallback;
    void *commandArg;
    uint32_t unk0x74;
    uint32_t blockSizeRegVal;
    uint32_t unk0x7C;
    uint32_t unk0x80;
    uint32_t unk0x84;
    void *unregisterCallback;
    void *unregisterArg;
};
CHECK_SIZE(MMCPhysDevice, 0x90);

struct MMCCommandRequest {
    int32_t deviceId;
    uint32_t opcode;
    uint32_t arg;
    uint32_t flags;
    uint32_t blocks;
    uint32_t blocksize;
    uint32_t timeout;
};

struct MMCCommandResponse {
    IPCRequest_t* request;
    MMCCommandResult result;
};

typedef void (*MMCCommandCallbackFn)(void *arg, MMCCommandResult *result);

MMCDevice *MMCGetDevice(int32_t deviceId);

void MMCPrepareCommand(MMCCommand *command);

int32_t MMCExecuteCommand(MMCDevice *device, MMCCommand *command, MMCDmaOpts *dma, MMCCommandResult *res, uint32_t status);

int32_t MMCPhysStartCommand(int32_t physDeviceId, MMCCommand *command, MMCDmaOpts *dma, MMCCommandCallbackFn callback, void *arg);

uint32_t MMCConvertTimeout(uint32_t access_time, uint32_t timeout);

int FSA_Ioctlv_MMC_Command(IPCRequest_t* request, IOSVec_t* vecs);
