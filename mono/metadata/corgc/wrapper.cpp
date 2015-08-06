#include <mono/metadata/corgc/wrapper.h>

// TODO Figure out if we need to do some of these operations by using interlocked
// TODO

void GCMonoObjectWrapper::RawSetMethodTable(GCMonoVTableWrapper* table)
{
    vtable = table;
}

MonoVTable* GCMonoObjectWrapper::RawGetMethodTable() const
{
    return vtable;
}

GCMonoObjectWrapper* GCMonoObjectWrapper::GetHeader()
{
    return this;
}

DWORD GCMonoObjectWrapper::GetNumComponents()
{
    return mono_array_length((MonoArray*)this);
}

void GCMonoObjectWrapper::Validate(BOOL bDeep, BOOL bVerifyNextHeader)
{
    // TODO
}

void GCMonoObjectWrapper::ValidatePromote(ScanContext* sc, DWORD flags)
{
    Validate();
}

void GCMonoObjectWrapper::ValidateHeap(GCMonoObjectWrapper *from, BOOL bDeep)
{
    Validate(bDeep, FALSE);
}

ADIndex GCMonoObjectWrapper::GetAppDomainIndex()
{
    return (ADIndex)RH_DEFAULT_DOMAIN_ID;
}

MonoVTable* GCMonoObjectWrapper::GetMethodTable() const
{
    return (MonoVTable*) (((size_t) RawGetMethodTable()) & (~(BIT_MASK)));
}

void GCMonoObjectWrapper::SetMarked()
{
    SetBit(BIT_MARKED);
}

BOOL GCMonoObjectWrapper::IsMarked() const
{
    return (GetBits() & BIT_MARKED) == BIT_MARKED;
}

void GCMonoObjectWrapper::SetPinned()
{
    // TODO interlocked
    SetBit(BIT_PINNED);
    return;
}

BOOL GCMonoObjectWrapper::IsPinned() const
{
    return (GetBits() & BIT_PINNED) == BIT_PINNED;
}

void GCMonoObjectWrapper::ClearMarked()
{
    ClrBit(BIT_MARKED);
}

CGCDesc* GCMonoObjectWrapper::GetSlotMap()
{
    // TODO
    return NULL;
}

void GCMonoObjectWrapper::SetFree(size_t size)
{
    /*assert(size >= free_object_base_size);

    assert(g_pFreeObjectMethodTable->GetBaseSize() == free_object_base_size);
    assert(g_pFreeObjectMethodTable->RawGetComponentSize() == 1);

    RawSetMethodTable(g_pFreeObjectMethodTable);*/

    // TODO
    return;
}

void GCMonoObjectWrapper::UnsetFree()
{
    memset(this, 0, sizeof(MonoObject));
}

BOOL GCMonoObjectWrapper::IsFree() const
{
    // TODO
    return FALSE;
}

int GCMonoObjectWrapper::GetRequiredAlignment() const
{
    return mono_class_min_align(vtable->klass);
}

BOOL GCMonoObjectWrapper::ContainsPointers() const
{
    return vtable->klass->has_references;
}

BOOL GCMonoObjectWrapper::Collectible() const
{
    return FALSE;
}

void GCMonoObjectWrapper::SetBit(DWORD bit)
{
    g_assert((bit & BIT_MASK) == bit);

    RawSetMethodTable((GCMonoVTableWrapper*)((size_t)RawGetMethodTable() | bit));
}

DWORD GCMonoObjectWrapper::GetBits() const
{
    return (size_t)RawGetMethodTable() & BIT_MASK;
}

void GCMonoObjectWrapper::ClrBit(DWORD bit)
{
    g_assert((bit & BIT_MASK) == bit);

    RawSetMethodTable((GCMonoVTableWrapper*)((size_t)RawGetMethodTable() & ~bit));
}
