#ifndef CORGC_WRAPPER_H_
#define CORGC_WRAPPER_H_

#include <mono/metadata/object.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/class-internals.h>

#define BITS_MASK 0xffff
#define BIT_MARKED 0x1
#define BIT_PINNED 0x2
#define BIT_FINALIZER_RUN 0x4

class CGDesc;
struct ScanContext;

class GCMonoVTableWrapper : public MonoVTable
{
protected:
        GCMonoVTableWrapper(){};
        ~GCMonoVTableWrapper(){};
public:
        DWORD GetBaseSize()
        {
                return mono_class_instance_size(klass);
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
        DWORD GetBits();
        void ClrBit(DWORD bit);
};


#endif /*CORGC_WRAPPER_H_*/
