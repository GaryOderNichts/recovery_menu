/* PROVIDE(printf = 0x0503dcc0); */
PROVIDE(printf = 0x05055438);
PROVIDE(memcmp = 0x05054d6c);
PROVIDE(memcpy = 0x05054e54);
PROVIDE(memset = 0x05054ef0);
PROVIDE(memmove = 0x05054f98);
PROVIDE(vsnprintf = 0x05055c40);
PROVIDE(snprintf = 0x05055c8c);
PROVIDE(strncpy = 0x05055db4);
PROVIDE(strnlen = 0x05055e94);
PROVIDE(strncmp = 0x05055e10);
PROVIDE(usleep = 0x050564e4);

PROVIDE(bspGetHardwareVersion = 0x0503d6e8);
PROVIDE(bspInit = 0x0503d35c);
PROVIDE(bspWrite = 0x0503d460);
PROVIDE(bspRead = 0x0503d550);

PROVIDE(UCWriteSysConfig = 0x05044a8c);
PROVIDE(UCReadSysConfig = 0x05044d5c);
PROVIDE(UCClose = 0x05045024);
PROVIDE(UCOpen = 0x05045028);

PROVIDE(IOS_CreateThread = 0x050567ec);
PROVIDE(IOS_JoinThread = 0x050567f4);
PROVIDE(IOS_CancelThread = 0x050567fc);
PROVIDE(IOS_StartThread = 0x05056824);
PROVIDE(IOS_GetThreadPriority = 0x0505683c);

PROVIDE(IOS_CreateMessageQueue = 0x0505684c);
PROVIDE(IOS_DestroyMessageQueue = 0x05056854);
PROVIDE(IOS_SendMessage = 0x0505685c);
PROVIDE(IOS_ReceiveMessage = 0x0505686c);

PROVIDE(IOS_CreateTimer = 0x05056884);
PROVIDE(IOS_RestartTimer = 0x0505688c);
PROVIDE(IOS_StopTimer = 0x05056894);
PROVIDE(IOS_DestroyTimer = 0x0505689c);

PROVIDE(IOS_CheckDebugMode = 0x050568ec);
PROVIDE(IOS_ReadOTP = 0x050568fc);

PROVIDE(IOS_HeapAlloc = 0x05056924);
PROVIDE(IOS_HeapAllocAligned = 0x0505692c);
PROVIDE(IOS_HeapFree = 0x05056934);
PROVIDE(IOS_HeapRealloc = 0x05056944);

PROVIDE(IOS_Open = 0x05056984);
PROVIDE(IOS_Close = 0x0505698c);
PROVIDE(IOS_Ioctl = 0x050569ac);
PROVIDE(IOS_Ioctlv = 0x050569b4);
PROVIDE(IOS_IoctlvAsync = 0x050569ec);

PROVIDE(IOS_InvalidateDCache = 0x05056a74);
PROVIDE(IOS_FlushDCache = 0x05056a7c);

PROVIDE(IOS_Shutdown = 0x05056b7c);
PROVIDE(IOS_Syscall0x81 = 0x05056bf4);

PROVIDE(ppcHeartBeatThreadId = 0x05070458);
PROVIDE(currentColdbootOS = 0x050b8174);
PROVIDE(currentColdbootTitle = 0x050b817c);

PROVIDE(IOSC_CreateObject = 0x050538b8);
PROVIDE(IOSC_Decrypt = 0x05054468);
PROVIDE(IOSC_DeleteObject = 0x05053820);
PROVIDE(IOSC_Encrypt = 0x05054808);
PROVIDE(IOSC_GenerateHash = 0x05053a6c);
PROVIDE(IOSC_GenerateRand = 0x05052228);
PROVIDE(IOSC_GetDeviceCertificate = 0x0505229c);
PROVIDE(IOSC_ImportSecretKey = 0x05053504);
PROVIDE(IOSC_ImportPublicKey = 0x050532c8);
