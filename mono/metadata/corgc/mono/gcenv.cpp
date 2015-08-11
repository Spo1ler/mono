#include "gcenv.h"

#define WIN32_MEM_COMMIT              0x1000
#define WIN32_MEM_RESERVE             0x2000
#define WIN32_MEM_DECOMMIT            0x4000
#define WIN32_MEM_RELEASE             0x8000
#define WIN32_MEM_RESET               0x80000

#define WIN32_PAGE_NOACCESS           0x01
#define WIN32_PAGE_READWRITE          0x04

#ifdef _WIN32
#include <WinBase.h>
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/param.h>
#if HAVE_SYS_VMPARAM_H
#include <sys/vmparam.h>
#endif  // HAVE_SYS_VMPARAM_H

#if HAVE_MACH_VM_TYPES_H
#include <mach/vm_types.h>
#endif // HAVE_MACH_VM_TYPES_H

#if HAVE_MACH_VM_PARAM_H
#include <mach/vm_param.h>
#endif  // HAVE_MACH_VM_PARAM_H
#endif // WIN32

#ifdef __APPLE__
#include <mach/vm_map.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#endif // __APPLE__

int32_t g_TrapReturningThreads;

bool g_fFinalizerRunOnShutDown;

#ifndef _WIN32

Thread* GetThread()
{
    // TODO
    return NULL;
}

void UnsafeInitializeCriticalSection(CRITICAL_SECTION *lpCriticalSection)
{
    // TODO
}

void UnsafeDeleteCriticalSection(CRITICAL_SECTION *lpCriticalSection)
{
    // TODO
}

void UnsafeEEEnterCriticalSection(CRITICAL_SECTION *lpCriticalSection)
{
    // TODO
}

void UnsafeEELeaveCriticalSection(CRITICAL_SECTION * lpCriticalSection)
{
    // TODO
}

void *_FastInterlockExchangePointer(void* volatile *target, void* value)
{
    void* result;

    do
    {
        result = *target;
    }
    while(InterlockedCompareExchangePointer(target, value, result) != result);

    return result;
}

void GetProcessMemoryLoad(LPMEMORYSTATUSEX lpBuffer)
{
    lpBuffer->dwLength = sizeof(MEMORYSTATUSEX);
    BOOL fRetVal = FALSE;
    memset(lpBuffer, 0, sizeof(MEMORYSTATUSEX));

#if HAVE_SYSCONF && HAVE__SC_PHYS_PAGES
    int64_t physical_memory;

    // Get the Physical memory size
    physical_memory = sysconf( _SC_PHYS_PAGES ) * sysconf( _SC_PAGE_SIZE );
    lpBuffer->ullTotalPhys = (DWORDLONG)physical_memory;
    fRetVal = TRUE;
#elif HAVE_SYSCTL
    int mib[2];
    int64_t physical_memory;
    size_t length;

    // Get the Physical memory size
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    length = sizeof(INT64);
    int rc = sysctl(mib, 2, &physical_memory, &length, NULL, 0);
    if (rc != 0)
    {
        ASSERT("sysctl failed for HW_MEMSIZE (%d)\n", errno);
    }
    else
    {
        lpBuffer->ullTotalPhys = (DWORDLONG)physical_memory;
        fRetVal = TRUE;
    }
#endif // HAVE_SYSCONF

    // Get the physical memory in use - from it, we can get the physical memory available.
    // We do this only when we have the total physical memory available.
    if (lpBuffer->ullTotalPhys > 0)
    {
#ifndef __APPLE__
        lpBuffer->ullAvailPhys = sysconf(SYSCONF_PAGES) * sysconf(_SC_PAGE_SIZE);
        INT64 used_memory = lpBuffer->ullTotalPhys - lpBuffer->ullAvailPhys;
        lpBuffer->dwMemoryLoad = (DWORD)((used_memory * 100) / lpBuffer->ullTotalPhys);
#else
        vm_size_t page_size;
        mach_port_t mach_port;
        mach_msg_type_number_t count;
        vm_statistics_data_t vm_stats;
        mach_port = mach_host_self();
        count = sizeof(vm_stats) / sizeof(natural_t);
        if (KERN_SUCCESS == host_page_size(mach_port, &page_size))
        {
            if (KERN_SUCCESS == host_statistics(mach_port, HOST_VM_INFO, (host_info_t)&vm_stats, &count))
            {
                lpBuffer->ullAvailPhys = (int64_t)vm_stats.free_count * (int64_t)page_size;
                INT64 used_memory = ((INT64)vm_stats.active_count + (INT64)vm_stats.inactive_count + (INT64)vm_stats.wire_count) *  (INT64)page_size;
                lpBuffer->dwMemoryLoad = (DWORD)((used_memory * 100) / lpBuffer->ullTotalPhys);
            }
        }
#endif // __APPLE__
    }

    // There is no API to get the total virtual address space size on
    // Unix, so we use a constant value representing 128TB, which is
    // the approximate size of total user virtual address space on
    // the currently supported Unix systems.
    static const UINT64 _128TB = (1ull << 47);
    lpBuffer->ullTotalVirtual = _128TB;
    lpBuffer->ullAvailVirtual = lpBuffer->ullAvailPhys;

    _ASSERTE(fRetVal);

    if(lpBuffer->ullAvailPhys > lpBuffer->ullTotalVirtual)
    {
        lpBuffer->ullAvailPhys = lpBuffer->ullAvailVirtual;
    }
}



inline uint32_t convertProtectionFlags(uint32_t win32flags)
{
    uint32_t result = 0;
    if((win32flags & WIN32_PAGE_NOACCESS) == WIN32_PAGE_NOACCESS)
    {
        result |= PROT_NONE;
    }
    if((win32flags & WIN32_PAGE_READWRITE) == WIN32_PAGE_READWRITE)
    {
        result |= (PROT_READ|PROT_WRITE);
    }

    return result;
}

inline uint32_t convertAllocationFlags(uint32_t win32flags)
{
    uint32_t result = 0;

    if((win32flags & WIN32_MEM_RESERVE) == WIN32_MEM_RESERVE)
    {
        result |= (MAP_PRIVATE|MAP_ANON);
    }
    if((win32flags & WIN32_MEM_COMMIT) == WIN32_MEM_COMMIT)
    {
        result |= (MAP_FIXED|MAP_SHARED|MAP_ANON);
    }
    // TODO

    return result;
}

#endif // _WIN32

void * ClrVirtualAlloc(
    void * lpAddress,
    size_t dwSize,
    uint32_t flAllocationType,
    uint32_t flProtect)
{
#ifdef _WIN32
        return VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
#else
        void* ptr = mmap(lpAddress,
                         dwSize,
                         convertProtectionFlags(flProtect),
                         convertAllocationFlags(flAllocationType),
                         -1, 0);

        if(ptr == MAP_FAILED)
        {
                return NULL;
        }

        return (msync(ptr, dwSize, MS_SYNC|MS_INVALIDATE) != -1) ? ptr : NULL;
#endif
}

void * ClrVirtualAllocAligned(
    void * lpAddress,
    size_t dwSize,
    uint32_t flAllocationType,
    uint32_t flProtect,
    size_t alignment)
{
    // Verify that the alignment is a power of 2
    _ASSERTE(alignment != 0);
    _ASSERTE((alignment & (alignment - 1)) == 0);

    dwSize += alignment;
    size_t addr = (size_t)ClrVirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
    return (void*)((addr + (alignment - 1)) & ~(alignment - 1));
}

bool ClrVirtualFree(
        void * lpAddress,
        size_t dwSize,
        uint32_t dwFreeType)
{
#ifdef _WIN32
        return VirtualFree(lpAddress, dwSize, dwFreeType);
#else
        bool success;
        if((dwFreeType & WIN32_MEM_DECOMMIT) == WIN32_MEM_DECOMMIT) // Decommit memory
        {
                success = mmap(lpAddress,
                               dwSize,
                               PROT_NONE,
                               MAP_FIXED|MAP_PRIVATE|MAP_ANON,
                               -1, 0)
                       != MAP_FAILED;
                if(success)
                {
                       return  msync(lpAddress, dwSize, MS_SYNC|MS_INVALIDATE) != -1;
                }
        }
        else // Free memory
        {
                success = msync(lpAddress, dwSize, MS_SYNC) != -1;
                if(success)
                {
                        return munmap(lpAddress, dwSize) != -1;
                }
        }

        return false;
#endif
}

bool
ClrVirtualProtect(
           void * lpAddress,
           size_t dwSize,
           uint32_t flNewProtect,
           uint32_t * lpflOldProtect)
{
#ifdef _WIN32
        return VirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect);
#else
        ASSERT(lpflOldProtect == NULL);
        return !mprotect(lpAddress, dwSize, convertProtectionFlags(flNewProtect));
#endif
}

bool ClrVirtualUnlock(
        void* lpAddress,
        size_t dwSize)
{
#ifdef _WIN32
    return VirtualUnlock(lpAddress, dwSize);
#else
    return munlock(lpAddress, dwSize) != -1;
#endif
}

#undef WIN32_MEM_COMMIT
#undef WIN32_MEM_DECOMMIT
#undef WIN32_MEM_RESERVE
#undef WIN32_MEM_RESET
#undef WIN32_PAGE_NOACCESS
#undef WIN32_PAGE_READWRITE
