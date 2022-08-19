#include "netconf.h"
#include "imports.h"
#include <string.h>

static int ifmgr_handle = -1;

int netconf_init(void)
{
    if(ifmgr_handle > 0) {
        return ifmgr_handle;
    }

    int ret = IOS_Open("/dev/net/ifmgr/ncl", 0);
    if(ret > 0) {
        ifmgr_handle = ret;
    }

    return ret;
}

int netconf_close(void)
{
    int ret = IOS_Close(ifmgr_handle);
    if (ret >= 0) {
        ifmgr_handle = -1;
    }

    return ret;
}

static void* allocIobuf(uint32_t size)
{
    void* ptr = IOS_HeapAlloc(CROSS_PROCESS_HEAP_ID, size);

    memset(ptr, 0, size);

    return ptr;
}

static void freeIobuf(void* ptr)
{
    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, ptr);
}

static int netconf_get_if_data(uint16_t* if_buf, uint16_t* data)
{
    if (!data) {
        return -2;
    }

    return IOS_Ioctl(ifmgr_handle, 0x14, if_buf, 2, data, 8);
}

NetConfLinkState netconf_get_if_linkstate(NetConfInterfaceType interface)
{
    uint16_t* buf = (uint16_t*) allocIobuf(0xa);
    buf[0] = interface;
    
    int res = netconf_get_if_data(buf, buf + 1);

    freeIobuf(buf);

    if (res >= 0) {
        return buf[2];
    }

    return res;
}

int netconf_get_assigned_address(NetConfInterfaceType interface, uint32_t* assignedAddress)
{
    uint16_t* buf = (uint16_t*) allocIobuf(8);
    buf[0] = interface;

    int res = IOS_Ioctl(ifmgr_handle, 0x16, buf, 2, buf + 2, 4);
    if (res >= 0) {
        memcpy(assignedAddress, buf + 2, 4);
    }

    freeIobuf(buf);

    return res;
}

int netconf_set_if_admin_state(NetConfInterfaceType interface, uint16_t admin_state)
{
    uint16_t* buf = (uint16_t*) allocIobuf(0x20);
    buf[0] = interface;
    buf[1] = admin_state;

    int res = IOS_Ioctl(ifmgr_handle, 0xf, buf, 0x20, NULL, 0);

    freeIobuf(buf);

    return res;
}

int netconf_get_startup_profile_id(void)
{
    return IOS_Ioctl(ifmgr_handle, 8, NULL, 0, NULL, 0);
}

int netconf_nv_load(uint32_t profileId)
{
    uint32_t* buf = allocIobuf(4);
    buf[0] = profileId;

    int res = IOS_Ioctl(ifmgr_handle, 6, buf, 4, NULL, 0);

    freeIobuf(buf);

    return res;
}

int netconf_get_running(NetConfCfg* running)
{
    uint32_t* buf = allocIobuf(0x280);

    int res = IOS_Ioctl(ifmgr_handle, 4, NULL, 0, buf, 0x280);
    if (res >= 0) {
        memcpy(running, buf, 0x280);
    }

    freeIobuf(buf);

    return res;
}

int netconf_set_running(NetConfCfg* config)
{
    uint32_t* buf = allocIobuf(0x280);
    memcpy(buf, config, 0x280);

    int res = IOS_Ioctl(ifmgr_handle, 3, buf, 0x280, NULL, 0);

    freeIobuf(buf);

    return res;
}

int netconf_set_eth_cfg(NetConfEthCfg* config)
{
    uint32_t* buf = allocIobuf(0x8);
    memcpy(buf, config, 0x8);

    int res = IOS_Ioctl(ifmgr_handle, 0x1a, buf, 0x8, NULL, 0);

    freeIobuf(buf);

    return res;
}

int netconf_set_wifi_cfg(NetConfWifiConfig* config)
{
    uint32_t* buf = allocIobuf(0x70);
    memcpy(buf, config, 0x70);

    int res = IOS_Ioctl(ifmgr_handle, 0x11, buf, 0x70, NULL, 0);

    freeIobuf(buf);

    return res;
}
