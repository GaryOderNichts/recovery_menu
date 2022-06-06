#pragma once

#include <stdint.h>
#include <assert.h>

// Thanks @Maschell for all the types
typedef uint16_t NetConfEthCfgSpeed;
typedef uint16_t NetConfEthCfgDuplex;
typedef uint16_t NetConfEthCfgNegotiation;
typedef uint16_t NetConfWifiPrivacyMode;
typedef uint16_t NetConfProxyAuthType;
typedef uint16_t NetConfProxyStatus;
typedef uint16_t NetConfInterfaceType;

typedef struct NetConfAllProfileState NetConfAllProfileState;
typedef struct NetConfDNSInfo NetConfDNSInfo;
typedef struct NetConfEthCfg NetConfEthCfg;
typedef struct NetConfProxyConfig NetConfProxyConfig;
typedef struct NetConfValidFlags NetConfValidFlags;
typedef struct NetConfWifiConfig NetConfWifiConfig;
typedef struct NetConfOpt NetConfOpt;
typedef struct NetConfIPv4Info NetConfIPv4Info;
typedef struct NetConfMACAddr NetConfMACAddr;
typedef struct NetConfCfg NetConfCfg;
typedef struct NetConfAOSSConfig NetConfAOSSConfig;
typedef struct NetConfWifiConfigManualPrivacy NetConfWifiConfigManualPrivacy;
typedef struct NetConfWifiConfigManual NetConfWifiConfigManual;
typedef struct NetConfInterface NetConfInterface;

typedef enum NetConfInterfaceTypeEnum {
    NET_CFG_INTERFACE_TYPE_WIFI = 0,
    NET_CFG_INTERFACE_TYPE_ETHERNET = 1,
}NetConfInterfaceTypeEnum;

typedef enum NetConfEthCfgSpeedEnum {
    NET_CFG_ETH_CFG_SPEED_10M = 10,
    NET_CFG_ETH_CFG_SPEED_100M = 100,
} NetConfEthCfgSpeedEnum;

typedef enum NetConfEthCfgDuplexEnum {
    NET_CFG_ETH_CFG_DUPLEX_HALF = 1,
    NET_CFG_ETH_CFG_DUPLEX_FULL = 2,
}NetConfEthCfgDuplexEnum;

typedef enum NetConfEthCfgNegotiationEnum {
    NET_CFG_ETH_CFG_NEGOTIATION_MANUAL = 1,
    NET_CFG_ETH_CFG_NEGOTIATION_AUTO = 2,
}NetConfEthCfgNegotiationEnum;

typedef enum NetConfIPv4Mode {
    NET_CONFIG_IPV4_MODE_AUTO_OBTAIN_IP = 0,
    NET_CONFIG_IPV4_MODE_NO_AUTO_OBTAIN_IP = 2,
}NetConfIPv4Mode;

typedef enum NetConfWifiPrivacyModeEnum {
    NET_CFG_WIFI_PRIVACY_MODE_NONE = 0,
    NET_CFG_WIFI_PRIVACY_MODE_WEP = 1,
    NET_CFG_WIFI_PRIVACY_MODE_WPA2_PSK_TKIP = 3,
    NET_CFG_WIFI_PRIVACY_MODE_WPA_PSK_TKIP = 4,
    NET_CFG_WIFI_PRIVACY_MODE_WPA2_PSK_AES = 5,
    NET_CFG_WIFI_PRIVACY_MODE_WPA_PSK_AES = 6,
}NetConfWifiPrivacyModeEnum;

typedef enum NetConfProxyAuthTypeEnum {
    NET_CFG_PROXY_AUTH_TYPE_NONE = 0,
    NET_CFG_PROXY_AUTH_TYPE_BASIC_AUTHENTICATION = 1,
}NetConfProxyAuthTypeEnum;

typedef enum NetConfProxyStatusEnum {
    NET_CFG_PROXY_DISABLED = 0,
    NET_CFG_PROXY_ENABLED = 1,
}NetConfProxyStatusEnum;

typedef enum NetConfLinkState {
    NET_CFG_LINK_STATE_UP = 1,
    NET_CFG_LINK_STATE_NO_USED = 2,
    NET_CFG_LINK_STATE_DOWN = 3,
}NetConfLinkState;

struct NetConfAllProfileState {
    char unk[0x18];
};

struct NetConfDNSInfo {
    char unk[0xE];
};

struct __attribute__((packed)) NetConfEthCfg {
    NetConfEthCfgSpeed speed;
    NetConfEthCfgDuplex duplex;
    NetConfEthCfgNegotiation negotiation;
    char padding[2];
};

struct NetConfIPv4Info {
    NetConfIPv4Mode mode;
    uint32_t addr;
    uint32_t netmask;
    uint32_t nexthop; // gateway
    uint32_t ns1; // dns 1
    uint32_t ns2; // dns 2
};

struct NetConfMACAddr {
    uint8_t MACAddr[0x6];
};

struct NetConfProxyConfig {
    NetConfProxyStatus use_proxy; // true/false
    uint16_t port;
    char padding[2];
    NetConfProxyAuthType auth_type;
    char host[0x80];
    char username[0x80]; // only 0x20 bytes usable
    char password[0x40]; // only 0x20 bytes usable
    char noproxy_hosts[0x80]; // not used
};

struct NetConfValidFlags {
    char unk[0x18];
};

struct __attribute__((packed)) NetConfWifiConfigManualPrivacy {
    NetConfWifiPrivacyMode mode;
    char padding[2];
    uint16_t aes_key_len;
    uint8_t aes_key[0x40];
    char padding2[2];
};

struct __attribute__((packed)) NetConfWifiConfigManual {
    uint8_t ssid[0x20];
    uint16_t ssidlength;
    char padding[2];
    NetConfWifiConfigManualPrivacy privacy;
};

struct NetConfWifiConfig {
    uint16_t config_method;
    char padding[2];
    NetConfWifiConfigManual config;
};

struct NetConfOpt {
    char unk[0x2c1];
};

struct __attribute__((packed)) NetConfInterface {
    uint16_t if_index;
    uint16_t if_sate;
    uint32_t if_mtu;
    NetConfIPv4Info ipv4Info;
};

struct NetConfCfg {
    NetConfInterface wl0;
    NetConfWifiConfig wifi;
    NetConfInterface eth0;
    NetConfEthCfg ethCfg;
    NetConfProxyConfig proxy;
};
static_assert(sizeof(NetConfCfg) == 0x280, "NetConfCfg: different size than expected");

struct NetConfAOSSConfig {
    char unk[0x1b0];
};

int netconf_init(void);

int netconf_close(void);

NetConfLinkState netconf_get_if_linkstate(NetConfInterfaceType interface);

int netconf_get_assigned_address(NetConfInterfaceType interface, uint32_t* assignedAddress);

int netconf_set_if_admin_state(NetConfInterfaceType interface, uint16_t admin_state);

int netconf_get_startup_profile_id(void);

int netconf_nv_load(uint32_t profileId);

int netconf_get_running(NetConfCfg* running);

int netconf_set_running(NetConfCfg* config);

int netconf_set_eth_cfg(NetConfEthCfg* config);

int netconf_set_wifi_cfg(NetConfWifiConfig* config);
