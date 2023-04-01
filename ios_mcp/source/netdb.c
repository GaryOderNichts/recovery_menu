#include "netdb.h"
#include "imports.h"

#include <string.h>

int h_errno;

struct dns_querys {
    struct dns_querys * next;
    uint32_t field1_0x4;
    uint32_t send_time;
    uint32_t expire_time;
    uint16_t tries;
    uint16_t lport;
    uint16_t id;
    uint16_t server_index;
    uint32_t fhost;
    int32_t replies;
    int32_t ipaddrs;
    uint32_t ipaddr_list[10];
    char* addrptrs[10];
    int32_t err;
    int32_t rcode;
    char dns_names[256];
    char ptr_name[256];
    uint32_t auths_ip;
    char* alist[4];
    struct hostent hostent;
    uint8_t type;
    uint8_t padding[3];
    uint32_t field27_0x2a8;
    uint32_t field28_0x2ac;
    void* dns_req;
    uint32_t has_next;
    //! self pointer used for pointer offset calculation
    void* query_ios_ptr;
};
static_assert(sizeof(struct dns_querys) == 0x2bc, "dns_querys: different size than expected");

struct dns_query_params {
    char name[128];
    uint8_t type;
    uint8_t padding[3];
    uint32_t unk0;
    uint32_t addr;
    uint32_t ttl; 
};
static_assert(sizeof(struct dns_query_params) == 0x90, "dns_query_params: different size than expected");

struct dns_ioctlv {
    IOSVec_t vecs[2];
    uint8_t padding0[104];
    struct dns_query_params params;
    uint8_t padding1[112];
    struct dns_querys query;
    uint8_t padding2[68];
};
static_assert(sizeof(struct dns_ioctlv) == 0x480, "dns_ioctlv: different size than expected");

static int do_dns_query(const char* name, uint8_t type, uint32_t addr, uint32_t ttl, struct dns_querys* query)
{
    struct dns_ioctlv* ioctlv = IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, sizeof(struct dns_ioctlv), 0x40);
    if (!ioctlv) {
        return -1;
    }

    memset(ioctlv, 0, sizeof(*ioctlv));

    strncpy(ioctlv->params.name, name, sizeof(ioctlv->params.name));
    ioctlv->params.type = type;
    ioctlv->params.ttl = ttl;
    ioctlv->params.addr = addr;

    ioctlv->vecs[0].ptr = &ioctlv->params;
    ioctlv->vecs[0].len = sizeof(ioctlv->params);
    ioctlv->vecs[1].ptr = &ioctlv->query;
    ioctlv->vecs[1].len = sizeof(ioctlv->query);

    int32_t res = IOS_Ioctlv(socketInit(), 0x26, 1, 1, ioctlv->vecs);

    memcpy(query, &ioctlv->query, sizeof(*query));
    
    // translate pointers
    query->hostent.h_aliases = query->alist;
    for (int i = 0; i < 2; i++) {
        if (query->alist[i]) {
            query->alist[i] = (char*) ((uint32_t) query + ((uint32_t) query->alist[i] - (uint32_t) query->query_ios_ptr));
        }
    }
    query->hostent.h_name = (char*) ((uint32_t) query + ((uint32_t) query->hostent.h_name - (uint32_t) query->query_ios_ptr));

    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, ioctlv);
    return res;
}

struct hostent* gethostbyname(const char* name)
{
    if (!name || name[0] == '\0') {
        h_errno = 3;
        return NULL;
    }

    static struct dns_querys dns_entry;
    do_dns_query(name, 1, 0, 0, &dns_entry);
    if (!dns_entry.ipaddrs) {
        h_errno = 1;
        return NULL;
    }

    dns_entry.hostent.h_addrtype = AF_INET;
    dns_entry.hostent.h_length = 4;
    dns_entry.hostent.h_addr_list = dns_entry.addrptrs;

    int i;
    for (i = 0; i < dns_entry.ipaddrs; i++) {
        dns_entry.addrptrs[i] = (char*) &dns_entry.ipaddr_list[i];
    }
    dns_entry.addrptrs[i] = NULL;

    h_errno = 0;
    return &dns_entry.hostent;
}
