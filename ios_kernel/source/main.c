#include "imports.h"
#include "thread.h"
#include "lolserial.h"
#include "../../ios_mcp/ios_mcp_syms.h"

extern char __kernel_bss_start;
extern char __kernel_bss_end;

extern char svcAB_handler;

extern uint32_t domainAccessPermissions[];

int kernel_syscall_0x81(int type, uint32_t address, uint32_t value)
{
    int res = 0;
    int level = disable_interrupts();
    set_domain_register(domainAccessPermissions[0]); // 0 = KERNEL

    if (type == 0) { // kernRead32
        res = *(volatile uint32_t*) address;
    } else if (type == 1) { // kernWrite32
        *(volatile uint32_t*) address = value;
    } else if (type == 2) { // readOtp
        res = readOTP(0, (void*) address, value);
    }

    set_domain_register(domainAccessPermissions[currentThreadContext->pid]);
    enable_interrupts(level);

    return res;
}

int _main(void* arg)
{
    lolserial_printf("Hello world from recovery_menu. Running from '%s'.\n", arg);

    int level = disable_interrupts();
    uint32_t control_register = disable_mmu();

    // clear all bss
    memset(&__kernel_bss_start, 0, &__kernel_bss_end - &__kernel_bss_start);
    memset((void*) (__mcp_bss_start - 0x05074000 + 0x08234000), 0, __mcp_bss_end - __mcp_bss_start);

    // map the mcp sections
    ios_map_shared_info_t map_info;
    map_info.paddr  = 0x050bd000 - 0x05000000 + 0x081c0000;
    map_info.vaddr  = 0x050bd000;
    map_info.size   = 0x3000;
    map_info.domain = 1; // MCP
    map_info.type   = 3;
    map_info.cached = 0xffffffff;
    _iosMapSharedUserExecution(&map_info);

    map_info.paddr  = 0x05116000 - 0x05100000 + 0x13d80000;
    map_info.vaddr  = 0x05116000;
    map_info.size   = 0x6000; // TODO how much can we actually map here?
    map_info.domain = 1; // MCP
    map_info.type   = 3;
    map_info.cached = 0xffffffff;
    _iosMapSharedUserExecution(&map_info);

    // redirect __sys_write0 to lolserial
    *(volatile uint32_t*) 0x0812dd68 = ARM_B(0x0812dd68, (uint32_t) &svcAB_handler);

    // add mcp ioctl hook to start mcp thread
    *(volatile uint32_t*) (0x05025242 - 0x05000000 + 0x081c0000) = THUMB_BL(0x05025242, _MCP_ioctl100_patch);

    // replace custom kernel syscall
    *(volatile uint32_t*) 0x0812cd2c = ARM_B(0x0812cd2c, kernel_syscall_0x81);

    restore_mmu(control_register);

    // invalidate all cache
    invalidate_dcache(NULL, 0x4001);
    invalidate_icache();

    enable_interrupts(level);

    // give the current thread full access to MCP for starting the thread
    setClientCapabilities(currentThreadContext->pid, 0xd, 0xffffffffffffffffllu);

    // start mcp thread
    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle > 0) {
        lolserial_printf("Starting MCP thread...\n");
        IOS_Ioctl(mcpHandle, 100, NULL, 0, NULL, 0);

        IOS_Close(mcpHandle);
    } else {
        lolserial_printf("Cannot open MCP: %x\n", mcpHandle);
    }

    return 0;
}
