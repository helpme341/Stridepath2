#pragma once
       
#include "UObject/Interface.h"
#include "UObject/ScriptInterface.h"
#include "UObject/StrongObjectPtr.h" 
#include "Templates/Casts.h"

template <typename T>
class TStrongInterfacePtr
{
    static_assert(TIsDerivedFrom<T, IInterface>::IsDerived, "TStrongInterfacePtr can only be used with UE interfaces");

public:
    TStrongInterfacePtr() = default;

    TStrongInterfacePtr(const TStrongInterfacePtr&) = delete;
    TStrongInterfacePtr& operator=(const TStrongInterfacePtr&) = delete;

    TStrongInterfacePtr(TStrongInterfacePtr&& Other) noexcept
        : Object(MoveTemp(Other.Object))
        , CachedInterface(Other.CachedInterface)
    {
        Other.CachedInterface = nullptr;
    }

    TStrongInterfacePtr& operator=(TStrongInterfacePtr&& Other) noexcept
    {
        if (this != &Other)
        {
            Object = MoveTemp(Other.Object);
            CachedInterface = Other.CachedInterface;
            Other.CachedInterface = nullptr;
        }
        return *this;
    }

    explicit TStrongInterfacePtr(UObject* InObject) { Reset(InObject); }

    explicit TStrongInterfacePtr(const TScriptInterface<T>& InScriptInterface)
    {
        Reset(InScriptInterface.GetObject());
    }

    ~TStrongInterfacePtr() { Reset(); }

    FORCEINLINE T* operator->() const { return CachedInterface; }
    FORCEINLINE explicit operator bool() const { return IsValid(); }

    FORCEINLINE T*       GetInterface() const { return CachedInterface; }
    FORCEINLINE UObject* GetObject()    const { return Object.Get(); }
    FORCEINLINE UObject* Get()          const { return Object.Get(); }

    FORCEINLINE T* GetInterfaceSafe() const
    {
        check(IsValid());
        return CachedInterface;
    }
    FORCEINLINE UObject* GetObjectSafe() const
    {
        check(IsValid());
        return Object.Get();
    }

    FORCEINLINE TScriptInterface<T> GetScriptInterface() const
    {
        TScriptInterface<T> Out;
        if (IsValid())
        {
            Out.SetObject(Object.Get());
            Out.SetInterface(CachedInterface);
        }
        return Out;
    }

    FORCEINLINE void Reset()
    {
        Object.Reset();
        CachedInterface = nullptr;
    }

    FORCEINLINE void Reset(UObject* InObject)
    {
        if (!InObject)
        {
            Reset();
            return;
        }
        Object = TStrongObjectPtr(InObject);
        CachedInterface = Cast<T>(InObject);
        if (!CachedInterface) Reset();
    }

    FORCEINLINE bool IsValid() const
    {
        return Object.IsValid() && (CachedInterface != nullptr);
    }

private:
    TStrongObjectPtr<UObject> Object;
    T* CachedInterface = nullptr;
};
