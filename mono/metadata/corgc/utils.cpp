// Bringing PAL along for the ride was too much of a pain due to the include hell
// that is going on with window types, which are defined like in every single place you go

#include "utils.h"
#include <pthread.h>

#ifdef __sparc__
VIRTUAL_PAGE_SIZE       = 0x2000,
#else   // __sparc__
        VIRTUAL_PAGE_SIZE       = 0x1000,
#endif  // __sparc__

//
// Helper memory page used by the FlushProcessWriteBuffers
//
static int s_helperPage[VIRTUAL_PAGE_SIZE / sizeof(int)] __attribute__((aligned(VIRTUAL_PAGE_SIZE)));

//
// Mutex to make the FlushProcessWriteBuffersMutex thread safe
//
pthread_mutex_t flushProcessWriteBuffersMutex;

/*++
  Function:
  InitializeFlushProcessWriteBuffers

  Abstract
  This function initializes data structures needed for the FlushProcessWriteBuffers
  Return
  TRUE if it succeeded, FALSE otherwise
  --*/
BOOL InitializeFlushProcessWriteBuffers()
{
        // Verify that the s_helperPage is really aligned to the VIRTUAL_PAGE_SIZE
        _ASSERTE((((SIZE_T)s_helperPage) & (VIRTUAL_PAGE_SIZE - 1)) == 0);

        // Locking the page ensures that it stays in memory during the two mprotect
        // calls in the FlushProcessWriteBuffers below. If the page was unmapped between
        // those calls, they would not have the expected effect of generating IPI.
        int status = mlock(s_helperPage, VIRTUAL_PAGE_SIZE);

        if (status != 0)
        {
                return FALSE;
        }

        status = pthread_mutex_init(&flushProcessWriteBuffersMutex, NULL);
        if (status != 0)
        {
                munlock(s_helperPage, VIRTUAL_PAGE_SIZE);
        }

        return status == 0;
}

void CORGC_UTILS_API FlushProcessWriteBuffers()
{
int status = pthread_mutex_lock(&flushProcessWriteBuffersMutex);
    FATAL_ASSERT(status == 0, "Failed to lock the flushProcessWriteBuffersMutex lock");

    // Changing a helper memory page protection from read / write to no access
    // causes the OS to issue IPI to flush TLBs on all processors. This also
    // results in flushing the processor buffers.
    status = mprotect(s_helperPage, VIRTUAL_PAGE_SIZE, PROT_READ | PROT_WRITE);
    FATAL_ASSERT(status == 0, "Failed to change helper page protection to read / write");

    // Ensure that the page is dirty before we change the protection so that
    // we prevent the OS from skipping the global TLB flush.
    InterlockedIncrement(s_helperPage);

    status = mprotect(s_helperPage, VIRTUAL_PAGE_SIZE, PROT_NONE);
    FATAL_ASSERT(status == 0, "Failed to change helper page protection to no access");

    status = pthread_mutex_unlock(&flushProcessWriteBuffersMutex);
    FATAL_ASSERT(status == 0, "Failed to unlock the flushProcessWriteBuffersMutex lock");
}
