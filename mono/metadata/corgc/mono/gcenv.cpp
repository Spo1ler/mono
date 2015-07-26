#include "mono/gcenv.h"

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
#endif // WIN32

#ifndef _WIN32

inline uint32_t convertProtectionFlags(uint32_t win32flags)
{
    uint32_t result = 0;
    if((win32flags & WIN32_PAGE_NOACCESS) = WIN32_PAGE_NOACCESS)
    {
        result |= PROT_NONE;
    }
    if((win32flags & WIN32_PAGE_READWRITE) = WIN32_PAGE_READWRITE)
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
        result |= (MAP_PRIVATE|MAP_ANONYMOUS);
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

#endif // !FEATURE_PAL
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
                successs = mmap(lpAddress,
                                dwSize,
                                PROT_NONE,
                                MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS,
                                -1, 0)
                        != MAP_FAILED);
                if(success)
                {
                       return  msync(lpAddress, dwSize, MS_SYNC|MS_INVALIDATE) != -1;
                }
        }
        else // Free memory
        {
                success = msync(lpAddress, size, MS_SYNC) != -1;
                if(success)
                {
                        return munmap(lpAddress, dwSize) != -1;
                }
        }

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

#undefine WIN32_MEM_COMMIT
#undefine WIN32_MEM_DECOMMIT
#undefine WIN32_MEM_RESERVE
#undefine WIN32_MEM_RESET
#undefine WIN32_PAGE_NOACCESS
#undefine WIN32_PAGE_READWRITE
