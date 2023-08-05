#pragma once

#include <stdint.h>
#include <stdio.h>

#define LOCAL_PROCESS_HEAP_ID 0xcafe
#define CROSS_PROCESS_HEAP_ID 0xcaff

typedef struct {
    void* ptr;
    uint32_t len;
    uint32_t paddr;
} IOSVec_t;

enum {
    UC_DATA_TYPE_U8      = 1,
    UC_DATA_TYPE_U16     = 2,
    UC_DATA_TYPE_U32     = 3,
    UC_DATA_TYPE_I32     = 4,
    UC_DATA_TYPE_F32     = 5,
    UC_DATA_TYPE_STRING  = 6,
    UC_DATA_TYPE_BINARY  = 7,
    UC_DATA_TYPE_COMPLEX = 8,
};

typedef struct __attribute__((__packed__)) {
    char name[64];
    uint32_t access;
    uint32_t data_type;
    int error;
    uint32_t data_size;
    void* data;
} UCSysConfig_t;

// thumb functions can't just be provided to the linker
#define setDefaultTitleId ((int (*)(uint64_t tid)) (0x0510d984 | 1))
#define encryptPrsh ((int (*)(uint32_t, uint32_t, void *, uint32_t)) (0x0500a610 | 1))

typedef enum BSPHardwareVersions {
    BSP_HARDWARE_VERSION_UNKNOWN                    = 0x00000000,
    BSP_HARDWARE_VERSION_HOLLYWOOD_ENG_SAMPLE_1     = 0x00000001,
    BSP_HARDWARE_VERSION_HOLLYWOOD_ENG_SAMPLE_2     = 0x10000001,
    BSP_HARDWARE_VERSION_HOLLYWOOD_PROD_FOR_WII     = 0x10100001,
    BSP_HARDWARE_VERSION_HOLLYWOOD_CORTADO          = 0x10100008,
    BSP_HARDWARE_VERSION_HOLLYWOOD_CORTADO_ESPRESSO = 0x1010000C,
    BSP_HARDWARE_VERSION_BOLLYWOOD                  = 0x20000001,
    BSP_HARDWARE_VERSION_BOLLYWOOD_PROD_FOR_WII     = 0x20100001,
    BSP_HARDWARE_VERSION_LATTE_A11_EV               = 0x21100010,
    BSP_HARDWARE_VERSION_LATTE_A11_CAT              = 0x21100020,
    BSP_HARDWARE_VERSION_LATTE_A12_EV               = 0x21200010,
    BSP_HARDWARE_VERSION_LATTE_A12_CAT              = 0x21200020,
    BSP_HARDWARE_VERSION_LATTE_A2X_EV               = 0x22100010,
    BSP_HARDWARE_VERSION_LATTE_A2X_CAT              = 0x22100020,
    BSP_HARDWARE_VERSION_LATTE_A3X_EV               = 0x23100010,
    BSP_HARDWARE_VERSION_LATTE_A3X_CAT              = 0x23100020,
    BSP_HARDWARE_VERSION_LATTE_A3X_CAFE             = 0x23100028,
    BSP_HARDWARE_VERSION_LATTE_A4X_EV               = 0x24100010,
    BSP_HARDWARE_VERSION_LATTE_A4X_CAT              = 0x24100020,
    BSP_HARDWARE_VERSION_LATTE_A4X_CAFE             = 0x24100028,
    BSP_HARDWARE_VERSION_LATTE_A5X_EV               = 0x25100010,
    BSP_HARDWARE_VERSION_LATTE_A5X_EV_Y             = 0x25100011,
    BSP_HARDWARE_VERSION_LATTE_A5X_CAT              = 0x25100020,
    BSP_HARDWARE_VERSION_LATTE_A5X_CAFE             = 0x25100028,
    BSP_HARDWARE_VERSION_LATTE_B1X_EV               = 0x26100010,
    BSP_HARDWARE_VERSION_LATTE_B1X_EV_Y             = 0x26100011,
    BSP_HARDWARE_VERSION_LATTE_B1X_CAT              = 0x26100020,
    BSP_HARDWARE_VERSION_LATTE_B1X_CAFE             = 0x26100028,
} BSPHardwareVersions;

int bspGetHardwareVersion(uint32_t* version);

int bspInit(const char* entity, uint32_t instance, const char* attribute, uint32_t size, const void* buffer);
int bspWrite(const char* entity, uint32_t instance, const char* attribute, uint32_t size, const void* buffer);
int bspRead(const char* entity, uint32_t instance, const char* attribute, uint32_t size, void* buffer);

int UCWriteSysConfig(int handle, uint32_t num, UCSysConfig_t* configs);
int UCReadSysConfig(int handle, uint32_t num, UCSysConfig_t* configs);
int UCClose(int handle);
int UCOpen(void);

int IOS_CreateThread(int (*fun)(void* arg), void* arg, void* stack_top, uint32_t stacksize, int priority, uint32_t flags);
int IOS_JoinThread(int threadid, int* retval);
int IOS_CancelThread(int threadid, int return_value);
int IOS_StartThread(int threadid);
int IOS_GetThreadPriority(int threadid);

int IOS_CreateMessageQueue(uint32_t* ptr, uint32_t n_msgs);
int IOS_DestroyMessageQueue(int queueid);
int IOS_ReceiveMessage(int queueid, uint32_t* message, uint32_t flags);

int IOS_CheckDebugMode(void);
int IOS_ReadOTP(int index, void* buffer, uint32_t size);

void* IOS_HeapAlloc(uint32_t heap, uint32_t size);
void* IOS_HeapAllocAligned(uint32_t heap, uint32_t size, uint32_t alignment);
void IOS_HeapFree(uint32_t heap, void* ptr);
void* IOS_HeapRealloc(uint32_t heap, void* ptr, uint32_t size);

int IOS_Open(const char* device, int mode);
int IOS_Close(int fd);
int IOS_Ioctl(int fd, uint32_t request, void* input_buffer, uint32_t input_buffer_len, void* output_buffer, uint32_t output_buffer_len);
int IOS_Ioctlv(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, IOSVec_t* vector);
int IOS_IoctlvAsync(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, IOSVec_t* vector, int callbackQueue, void* msg);

void IOS_InvalidateDCache(void* ptr, uint32_t len);
void IOS_FlushDCache(void* ptr, uint32_t len);

void IOS_Shutdown(int reset);
int IOS_Syscall0x81(int type, uint32_t address, uint32_t value);

// context: 0x70-byte buffer for chaining hash calls together
#define IOSC_HASH_CONTEXT_SIZE      0x70
#define IOSC_HASH_FLAGS_SHA1_INIT       0x000
#define IOSC_HASH_FLAGS_SHA1_UPDATE     0x001
#define IOSC_HASH_FLAGS_SHA1_FINALIZE   0x002
#define IOSC_HASH_FLAGS_SHA256_INIT     0x100
#define IOSC_HASH_FLAGS_SHA256_UPDATE   0x101
#define IOSC_HASH_FLAGS_SHA256_FINALIZE 0x102
int IOSC_GenerateHash(uint8_t* context, uint32_t contextSize, uint8_t* inputData, uint32_t inputSize, uint32_t flags, uint8_t* hashData, uint32_t outputSize);

typedef int IOSCPublicKeyHandle;
typedef int IOSCSecretKeyHandle;
typedef unsigned int IOSCSecretKeySecurity;

typedef unsigned int IOSCObjectType;
typedef unsigned int IOSCObjectSubType;

int IOSC_CreateObject(int* keyHandle, IOSCObjectType type, IOSCObjectSubType subtype);
int IOSC_DeleteObject(int keyHandle);
int IOSC_GenerateRand(uint8_t* randBytes, uint32_t numBytes);
int IOSC_ImportSecretKey(IOSCSecretKeyHandle importedHandle, IOSCSecretKeyHandle verifyHandle, IOSCSecretKeyHandle decryptHandle, IOSCSecretKeySecurity flag, uint8_t* signbuffer, uint32_t signbufferSize, uint8_t* ivData, uint32_t ivSize, uint8_t* keybuffer, uint32_t keybufferSize);
int IOSC_ImportPublicKey(uint8_t* publicKeyData, uint32_t dataSize, uint8_t* exponent, uint32_t exponentSize, IOSCPublicKeyHandle publicKeyHandle);
int IOSC_Decrypt(IOSCSecretKeyHandle decryptHandle, uint8_t* ivData, uint32_t ivSize, uint8_t* inputData, uint32_t inputSize, uint8_t* outputData, uint32_t outputSize);
int IOSC_Encrypt(IOSCSecretKeyHandle encryptHandle, uint8_t* ivData, uint32_t ivSize, uint8_t* inputData, uint32_t inputSize, uint8_t* outputData, uint32_t outputSize);

#define WUP_CERT_SIGTYPE_ECC_SHA1   0x00010002  /* ECC with SHA-1 */
#define WUP_CERT_SIGTYPE_ECC_SHA256 0x00010005  /* ECC with SHA-256 */
#define WUP_NG_ISSUER_PPKI "Root-CA00000003-MS00000012"
#define WUP_NG_ISSUER_DPKI "Root-CA00000002-MS00000003"
typedef struct IOSCEccSignedCert {
    uint32_t signature_type;    // [0x000] 0x00010002 or 0x00010005
    uint8_t signature[0x3C];    // [0x004] ECC signature
    uint8_t reserved1[0x40];    // [0x040] All zeroes
    char issuer[0x40];          // [0x080] Issuer
    uint32_t key_type;          // [0x0C0] Key type (2)
    char console_id[0x40];      // [0x0C4] Console ID ("NGxxxxxxxx")
    uint32_t ng_id;             // [0x104] NG ID
    uint8_t pub_key[0x3C];      // [0x108] ECC public key
    uint8_t reserved2[0x3C];    // [0x10C] All zeroes
} IOSCEccSignedCert;

int IOSC_GetDeviceCertificate(IOSCEccSignedCert* certificate, uint32_t certSize);
