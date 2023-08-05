#include "pm.h"
#include "imports.h"

int PM_Restart(void)
{
    int fd = IOS_Open("/dev/pm",0);
    if (fd < 0) {
        return fd;
    }

    int res = IOS_Ioctl(fd, 0xe3, NULL, 0, NULL, 0);
    IOS_Close(fd);

    return res;
}
