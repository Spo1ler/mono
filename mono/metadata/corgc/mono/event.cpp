#include "gcenv.h"
#include <mono/io-layer/wapi.h>

void CLREventStatic::CreateManualEvent(bool bInitialState)
{
    m_hEvent = CreateEvent(NULL, TRUE, bInitialState, NULL);
    m_fInitialized = true;
}

void CLREventStatic::CreateAutoEvent(bool bInitialState)
{
    m_hEvent = CreateEvent(NULL, FALSE, bInitialState, NULL);
    m_fInitialized = true;
}

void CLREventStatic::CreateOSManualEvent(bool bInitialState)
{
    m_hEvent = CreateEvent(NULL, TRUE, bInitialState, NULL);
    m_fInitialized = true;
}
void CLREventStatic::CreateOSAutoEvent(bool bInitialState)
{
    m_hEvent = CreateEvent(NULL, TRUE, bInitialState, NULL);
    m_fInitialized = true;
}

void CLREventStatic::CloseEvent()
{
    if(IsValid())
    {
        CloseHandle(m_hEvent);
        m_hEvent = INVALID_HANDLE_VALUE;
    }
}
bool CLREventStatic::IsValid() const
{
    return m_fInitialized && m_hEvent != INVALID_HANDLE_VALUE;
}

bool CLREventStatic::Set()
{
    if(!m_fInitialized)
        return false;
    return !!SetEvent(m_hEvent);
}

bool CLREventStatic::Reset()
{
    if(!m_fInitialized)
        return false;
    return !!ResetEvent(m_hEvent);
}
uint32_t CLREventStatic::Wait(uint32_t dwMilliseconds, bool bAlertable)
{
    uint32_t result = WAIT_FAILED;

    if(m_fInitialized)
    {
        bool disablePreemptive = false;
        Thread* pCurThread = GetThread();

        if(pCurThread != NULL && pCurThread->PreemptiveGCDisabled())
        {
            pCurThread->EnablePreemptiveGC();
            disablePreemptive = true;
        }

        result = WaitForSingleObjectEx(m_hEvent, dwMilliseconds, bAlertable);

        if(disablePreemptive)
        {
            pCurThread->DisablePreemptiveGC();
        }
    }

    return result;
}

HANDLE CLREventStatic::GetOSEvent()
{
    return m_hEvent;
}
