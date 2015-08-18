#ifndef CORGC_GC_H
#define CORGC_GC_H

#include <glib.h>
#include <pthread.h>

#include "config.h"
extern "C"{
#include <mono/metadata/runtime.h>
#include <mono/metadata/method-builder.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/class-internals.h>

#include <mono/utils/atomic.h>
#include <mono/utils/mono-threads.h>
#include <mono/utils/mono-counters.h>
}

#if SIZEOF_VOID_P == 4
typedef guint32 mword;
#else
typedef guint64 mword;
#endif

#define header(o) ((GCMonoObjectWrapper*)(o))
#define heap() GCHeap::GetGCHeap()

extern "C" mono_mutex_t gc_mutex;

inline void corgc_lock()
{
  mono_mutex_lock(&gc_mutex);
}

inline void corgc_unlock()
{
  mono_mutex_unlock(&gc_mutex);
}

#endif
