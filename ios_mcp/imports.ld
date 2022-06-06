/* PROVIDE(printf = 0x0503dcc0); */
PROVIDE(printf = 0x05055438);
PROVIDE(memcmp = 0x05054d6c);
PROVIDE(memcpy = 0x05054e54);
PROVIDE(memset = 0x05054ef0);
PROVIDE(vsnprintf = 0x05055c40);
PROVIDE(snprintf = 0x05055c8c);
PROVIDE(strncpy = 0x05055db4);
PROVIDE(strnlen = 0x05055e94);
PROVIDE(strncmp = 0x05055e10);
PROVIDE(usleep = 0x050564e4);

PROVIDE(bspWrite = 0x0503d460);
PROVIDE(bspRead = 0x0503d550);

PROVIDE(IOS_CreateThread = 0x050567ec);
PROVIDE(IOS_JoinThread = 0x050567f4);
PROVIDE(IOS_CancelThread = 0x050567fc);
PROVIDE(IOS_StartThread = 0x05056824);
PROVIDE(IOS_GetThreadPriority = 0x0505683c);

PROVIDE(IOS_CreateMessageQueue = 0x0505684c);
PROVIDE(IOS_DestroyMessageQueue = 0x05056854);
PROVIDE(IOS_ReceiveMessage = 0x0505686c);

PROVIDE(IOS_HeapAlloc = 0x05056924);
PROVIDE(IOS_HeapAllocAligned = 0x0505692c);
PROVIDE(IOS_HeapFree = 0x05056934);

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
