#pragma once

#include "imports.h"
#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint64_t titleId;
    char unk[14];
} MCPInstallInfo;

typedef struct __attribute__((packed)) {
   uint32_t inProgress;
   uint64_t titleId;
   uint64_t sizeTotal;
   uint64_t sizeProgress;
   uint32_t contentsTotal;
   uint32_t contentsProgress;
} MCPInstallProgress;

typedef struct __attribute__((packed)) {
    IOSIpcRequest_t reply;
    void* ioBuf;
} MCPAsyncReply;

int MCP_InstallGetInfo(int handle, const char* path, MCPInstallInfo* out_info);

int MCP_InstallSetTargetUsb(int handle, int target);

int MCP_InstallSetTargetDevice(int handle, int device);

int MCP_InstallTitle(int handle, const char* path);

int MCP_InstallTitleAsync(int handle, const char* path, int callbackQueue);

int MCP_InstallGetProgress(int handle, MCPInstallProgress* progress);
