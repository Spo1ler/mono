
#include "gcenv.h"

void GCToEEInterface::SuspendEE(SUSPEND_REASON reason)
{

}

void GCToEEInterface::RestartEE(bool bFinishedGC)
{

}

void GCToEEInterface::ScanStackRoots(Thread* pThread, promote_func* fn, ScanContext* sc)
{

}
void GCToEEInterface::ScanStaticGCRefsOpportunistically(promote_func* fn, ScanContext* sc)
{
}

void GCToEEInterface::GcStartWork(int condemned, int max_gen)
{
}

void GCToEEInterface::AfterGcScanRoots(int condemned, int max_gen, ScanContext* sc)
{
}

void GCToEEInterface::GcBeforeBGCSweepWork()
{
}

void GCToEEInterface::GcDone(int condemned)
{
}

void GCToEEInterface::SyncBlockCacheWeakPtrScan(HANDLESCANPROC scanProc, LPARAM lp1, LPARAM lp2)
{

}
void GCToEEInterface::SyncBlockCacheDemote(int max_gen)
{

}
void GCToEEInterface::SyncBlockCachePromotionsGranted(int max_gen)
{

}
