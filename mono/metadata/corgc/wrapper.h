#ifndef CORGC_WRAPPER_H_
#define CORGC_WRAPPER_H_

#include <mono/metadata/object.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/class-internals.h>
#include "mono/gcenv.h"

#define BITS_MASK 0xffff
#define BIT_MARKED 0x1
#define BIT_PINNED 0x2
#define BIT_FINALIZER_RUN 0x4

class CGDesc;
struct ScanContext;

// Not sure, in coreclr it was 2*sizeof(BYTE*) + sizeof(ObjHeader),
// but MonoObject has a sync block inside it rather than outside
#define MIN_OBJECT_SIZE     (sizeof(BYTE*) + sizeof(MonoObject))

class GCMonoVTableWrapper : public MonoVTable
{
protected:
    GCMonoVTableWrapper(){};
    ~GCMonoVTableWrapper(){};
public:
    DWORD GetBaseSize() const
    {
        return mono_class_instance_size(klass);
    }

    BOOL HasComponentSize() const
    {
        return klass->rank > 0;
    }

    WORD RawGetComponentSize() const
    {
        return mono_array_element_size(klass);
    }

    SIZE_T GetComponentSize() const
    {
        return HasComponentSize() ? RawGetComponentSize() : 0;
    }

    BOOL HasFinalizer() const
    {
        return mono_class_has_finalizer(klass);
    }

    BOOL HasCriticalFinalizer() const
    {
        return mono_class_has_parent_and_ignore_generics(klass, mono_defaults.critical_finalizer_object);
    }
};

class GCMonoObjectWrapper : public MonoObject
{
protected:
    GCMonoObjectWrapper(){};
    ~GCMonoObjectWrapper(){};
public:
    MonoVTable* RawGetMethodTable() const;
    DWORD GetNumComponents();

    GCMonoObjectWrapper *GetHeader();

    void Validate(BOOL bDeep=TRUE, BOOL bVerifyNextHeader=TRUE);
    void ValidatePromote(ScanContext *sc, DWORD flags);
    void ValidateHeap(MonoObject *from, BOOL bDeep);

    ADIndex GetAppDomainIndex();

    MonoVTable* GetMethodTable() const;

    void SetMarked();
    BOOL IsMarked() const;

    void SetPinned();
    BOOL IsPinned() const;

    void ClearMarked();

    CGDesc *GetSlotMap();

    void SetFree(size_t size);
    void UnsetFree();
    BOOL IsFree() const;

    int GetRequiredAlignment() const;
    BOOL ContainsPointers() const;

    BOOL Collectible() const;

    FORCEINLINE BOOL ContainsPointersOrCollectible() const
    {
        return ContainsPointers() || Collectible();
    }

    void SetBit(DWORD bit);
    DWORD GetBits() const;
    void ClrBit(DWORD bit);
    GCMonoObjectWrapper* GetObjectBase()
    {
        return this;
    }
};


#endif /*CORGC_WRAPPER_H_*/
