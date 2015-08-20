#include "gcenv.h"
Thread* ThreadStore::m_pThreadList = NULL;

Thread* ThreadStore::GetThreadList(Thread * pThread)
{
  if(pThread == NULL)
    return m_pThreadList;
  else
    return pThread->m_pNext;
}

void ThreadStore::AttachCurrentThread(bool fAcquireThreadStoreLock)
{
  MonoThreadInfo* info = mono_thread_info_current();
  if(m_pThreadList)
  {
    m_pThreadList->m_pNext = (CorgcThreadInfo*)info;
  }
  else
  {
    m_pThreadList = (CorgcThreadInfo*)info;
    m_pThreadList->m_pNext = NULL;
  }
}

void FinalizerThread::EnableFinalization()
{

}
