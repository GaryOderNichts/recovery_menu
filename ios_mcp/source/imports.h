#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct {
    void* ptr;
    uint32_t len;
    uint32_t paddr;
} IOSVec_t;

// thumb functions can't just be provided to the linker
#define setDefaultTitleId ((int (*)(uint64_t tid)) (0x0510d984 | 1))

int bspWrite(const char* entity, uint32_t instance, const char* attribute, uint32_t size, const void* buffer);
int bspRead(const char* entity, uint32_t instance, const char* attribute, uint32_t size, void* buffer);

int IOS_CreateThread(int (*fun)(void* arg), void* arg, void* stack_top, uint32_t stacksize, int priority, uint32_t flags);
int IOS_JoinThread(int threadid, int* retval);
int IOS_CancelThread(int threadid, int return_value);
int IOS_StartThread(int threadid);
int IOS_GetThreadPriority(int threadid);

int IOS_CreateMessageQueue(uint32_t* ptr, uint32_t n_msgs);
int IOS_DestroyMessageQueue(int queueid);
int IOS_ReceiveMessage(int queueid, uint32_t* message, uint32_t flags);

void* IOS_HeapAlloc(uint32_t heap, uint32_t size);
void* IOS_HeapAllocAligned(uint32_t heap, uint32_t size, uint32_t alignment);
void IOS_HeapFree(uint32_t heap, void* ptr);

int IOS_Open(const char* device, int mode);
int IOS_Close(int fd);
int IOS_Ioctl(int fd, uint32_t request, void* input_buffer, uint32_t input_buffer_len, void* output_buffer, uint32_t output_buffer_len);
int IOS_Ioctlv(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, IOSVec_t* vector);
int IOS_IoctlvAsync(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, IOSVec_t* vector, int callbackQueue, void* msg);

void IOS_InvalidateDCache(void* ptr, uint32_t len);
void IOS_FlushDCache(void* ptr, uint32_t len);

void IOS_Shutdown(int reset);
int IOS_Syscall0x81(int type, uint32_t address, uint32_t value);
