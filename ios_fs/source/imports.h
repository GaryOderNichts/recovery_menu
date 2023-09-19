#pragma once

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((__packed__))
#endif

#ifndef CHECK_SIZE
#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, #type " must be " #size " bytes")
#endif

typedef struct {
	void* ptr;
	uint32_t len;
	uint32_t paddr;
} IOSVec_t;

typedef struct IPCRequest_t IPCRequest_t;

void *AllocPool_Get(int32_t id);
int AllocPool_Ret(int32_t id);

int FSA_HandleIoctl(IPCRequest_t* request, int command, void* inBuf, void* outBuf);

int FSA_HandleIoctlv(IPCRequest_t* request, int command, IOSVec_t* vecs);

int IOS_ResourceReply(IPCRequest_t* request, int32_t result);

int IOS_WaitSemaphore(int id, uint32_t tryWait);
int IOS_SignalSemaphore(int id);
