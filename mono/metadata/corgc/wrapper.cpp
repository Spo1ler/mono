#include <mono/metadata/corgc/wrapper.h>

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
        return NULL;
}

void GCMonoObjectWrapper::Validate(BOOL bDeep, BOOL bVerifyNextHeader)
{
        // TODO
        if(this == NULL)
                return;

        BOOL fSmallObjectHeapPtr = FALSE, fLargeObjectHeapPtr = FALSE;

        fSmallObjectHeapPtr = GCHeap::GetGCHeap()->IsHeapPointer(this, TRUE);
        if(!fSmallObjectHeapPtr)
                fLargeObjectHeapPtr = GCHeap::GetGCHeap()->IsHeapPointer(this);

        /*_ASSERTE(fSmallObjectHeapPtr || fLargeObjectHeapPtr);*/
        /*_ASSERTE(GetMethodTable()->GetBaseSize() >= 8);

#ifdef FEATURE_STRUCTALIGN
        _ASSERTE(IsStructAligned((BYTE*)this, GetMethodTable()->GetBaseAlignment());
#endif // FEATURE_STRUCTALIGN

#ifdef VERIFY_HEAP
         if(bDeep)
             GCHeap::GetGCHeap()->ValidateObjectMember(this);
#endif
        */

}

void GCMonoObjectWrapper::ValidatePromote(ScanContext* sc, DWORD flags)
{
        Validate();
}

void GCMonoObjectWrapper::ValidateHeap(Object *from, BOOL bDeep)
{
        Validate(bDeeb, FALSE);
}

ADIndex GCMonoObjectWrapper::GetAppDomainIndex()
{
        return (ADIndex)RH_DEFAULT_DOMAIN_ID;
}

MonoVTable* GCMonoObjectWrapper::GetMethodTable() const
{
        return (MonoVTable*) (((size_t) RawGetMethodTable()) & (~(BIT_MARKED)));
}

void GCMonoObjectWrapper::SetMarked()
{
        SetBit(BIT_MARKED);
}

BOOL GCMonoObjectWrapper::IsMarked() const
{
        return GetBits() & BIT_MARKED == BIT_MARKED;
}

void GCMonoObjectWrapper::SetPinned()
{
        // TODO interlocked
        SetBit(BIT_PINNED);
        return;
}

BOOL GCMonoObjectWrapper::IsPinned() const
{
        return GetBits() & BIT _PINNED == BIT_PINNED;
}

void GCMonoObjectWrapper::ClearMarked()
{
        ClearBit(BIT_MARKED);
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

        return;

        // TODO
}

BOOL GCMonoObjectWrapper::IsFree() const
{
        // TODO
        return FALSE;
}

int GCMonoObjectWrapper::GetRequiredAlignment() const
{
        return (int)vtable->klass->min_align;
}

BOOL GCMonoObjectWrapper::ContainsPointers() const
{
        return !!(vtable->klass->has_references);
}

BOOL GCMonoObjectWrapper::Collectible() const
{
        return FALSE;
}

void GCMonoObjectWrapper::SetBit(DWORD bit)
{
        assert(bit & BITS_MASK == bit);

        RawSetMethodTable(RawGetMethodTable() | bit);
}

DWORD GCMonoObjectWrapper::GetBits()
{
        return RawGetMethodTable() & BITS_MASK;
}

void GCMonoObjectWrapper::ClrBit(DWORD bit)
{
        assert(bit & BITS_MASK == bit);

        RawSetMethodTable(RawGetMethodTable() & ~bit);
}
