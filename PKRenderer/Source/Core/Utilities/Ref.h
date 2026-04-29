#pragma once
#include "Memory.h"

namespace PK
{
    using nullptr_t = decltype(nullptr);
    template <typename, typename> struct Unique;
    template <typename> struct FixedUnique;
    template <typename> struct Ref;
    template <typename> struct Weak;

    template<typename T>
    struct DefaultDeleter
    {
        constexpr DefaultDeleter() noexcept = default;

        template<typename U>  
        DefaultDeleter(const DefaultDeleter<U>&, typename TEnableIf<__is_convertible_to(U*, T*)>::Type * = 0) noexcept {}

        void operator()(T* p) const noexcept { Memory::Delete(p); }
    };

    template<typename T, typename D, bool isEmpty = __is_empty(D) && !__is_final(D)>
    struct UniquePair
    {
        constexpr UniquePair() noexcept = default;
        UniquePair(T* ptr) : pointer(ptr) {}
        UniquePair(T* ptr, const D& del) : pointer(ptr), deleter(del) {}
        D& GetDeleter() { return deleter; }
        T* pointer = nullptr;
        D deleter{};
    };

    template<typename T, typename D>
    struct UniquePair<T, D, true> : public D
    {
        constexpr UniquePair() noexcept = default;
        UniquePair(T* ptr) : pointer(ptr) {}
        UniquePair(T* ptr, const D&) : pointer(ptr) {}
        D& GetDeleter() { return *this; }
        T* pointer = nullptr;
    };

    template <typename T, typename D = DefaultDeleter<T>>
    struct Unique
    {
        static_assert(TIsArray<T> == false, "Unique doesn't support array types.");

        typedef Unique<T, D>  this_type;

        constexpr Unique() noexcept : pair(nullptr) {}
        constexpr Unique(nullptr_t) noexcept : pair(nullptr) {}
        explicit Unique(T* ptr) noexcept : pair(ptr) {}
        Unique(T* ptr, D deleter) noexcept : pair(ptr, deleter) {}
        Unique(T* ptr, D&& deleter) noexcept : pair(ptr, PK::MoveTemp(deleter)) {}
        Unique(Unique&& other) noexcept : pair(other.Release(), PK::Forward<D>(other.GetDeleter())) {}
        Unique(const Unique&) = delete;
        
        template <typename U, typename E>
        Unique(Unique<U, E>&& other) noexcept : pair(other.Release(), PK::Forward<E>(other.GetDeleter())) {}

        ~Unique() noexcept { Reset(); }

        Unique& operator=(Unique&& other) noexcept
        {
            Reset(other.Release());
            pair.GetDeleter() = PK::MoveTemp(PK::Forward<D>(other.GetDeleter()));
            return *this;
        }

        template <typename U, typename E>
        Unique& operator=(Unique<U, E>&& other) noexcept
        {
            Reset(other.Release());
            pair.GetDeleter() = PK::MoveTemp(PK::Forward<E>(other.GetDeleter()));
            return *this;
        }
        
        Unique& operator=(nullptr_t) noexcept { Reset(); return *this; }
        Unique& operator=(const Unique&) = delete;
        Unique& operator=(T* pValue) = delete;
        const T& operator*() const { return *pair.pointer; }
        T& operator*() { return *pair.pointer; }
        T* operator->() const noexcept { return pair.pointer; }
        explicit operator bool() const noexcept { return pair.pointer != nullptr; }

        T* get() const noexcept { return pair.pointer; }
        const D& GetDeleter() const noexcept { return pair.GetDeleter(); }
        D& GetDeleter() noexcept { return pair.GetDeleter(); }

        void Reset(T* pValue = nullptr) noexcept
        {
            if (pValue != pair.pointer)
            {
                if (auto first = PK::Exchange(pair.pointer, pValue))
                {
                    GetDeleter()(first);
                }
            }
        }

        T* Release() noexcept
        {
            T* pTemp = pair.pointer;
            pair.pointer = nullptr;
            return pTemp;
        }

        void Swap(Unique& other) noexcept { PK::Swap(pair.pointer, other.pair.pointer); }

        UniquePair<T, D> pair;
    }; 

    template<typename T>
    struct FixedUnique
    {
        FixedUnique() noexcept : m_isCreated(false) {}
        FixedUnique(FixedUnique const&) = delete;
        ~FixedUnique() noexcept { Delete(); }
        FixedUnique& operator=(FixedUnique const&) = delete;

        const T* get() const  noexcept { return m_isCreated ? &value : nullptr; }
        T* get() noexcept { return m_isCreated ? &value : nullptr; }
        operator const T* () const noexcept { return get(); }
        operator T* () noexcept { return get(); }
        const T* operator -> () const noexcept { return get(); }
        T* operator -> () noexcept { return get(); }

        template<typename ... Args>
        void New(Args&& ... args) noexcept
        {
            if (!m_isCreated)
            {
                Memory::Construct(&value, PK::Forward<Args>(args)...);
                m_isCreated = true;
            }
        }

        void Delete() noexcept
        {
            if (m_isCreated)
            {
                Memory::Destruct(&value);
                m_isCreated = false;
            }
        }

    private:
        struct U { constexpr U() noexcept {} };
        union { U other; T value; };
        bool m_isCreated;
    };

    struct SharedObjectBase
    {
        constexpr SharedObjectBase() noexcept = default;
        SharedObjectBase(const SharedObjectBase&) = delete;
        SharedObjectBase& operator=(const SharedObjectBase&) = delete;
        virtual ~SharedObjectBase() noexcept {}
        virtual void Destroy() noexcept = 0;
        virtual void Delete() noexcept = 0;
        constexpr uint32_t GetStrongRefCount() const noexcept { return referenceCount; }
        constexpr uint32_t GetWeakRefCount() const noexcept { return weakCount; }
        void IncrementWeakRef() noexcept;
        void IncrementStrongRef() noexcept;
        void DecrementWeakRef() noexcept;
        void DecrementStrongRef() noexcept;
        bool IncrementRefNonZero() noexcept;
        
        uint32_t referenceCount = 1u;
        uint32_t weakCount = 1u;
    };

    template<typename T>
    struct SharedObject : public SharedObjectBase
    {
        template <typename ... Args>
        explicit SharedObject(Args&&... args) : SharedObjectBase() { Memory::Construct(&value, PK::Forward<Args>(args)...); }
        virtual ~SharedObject() noexcept override {}
        virtual void Destroy() noexcept override final { Memory::Destruct(&value); }
        virtual void Delete() noexcept override final { Memory::Delete(this); }
        struct U { constexpr U() noexcept {} };
        union { U unionDefault; T value; };
    };

    template<typename T>
    struct RefBase
    {
        RefBase(const RefBase&) = delete;
        RefBase& operator=(const RefBase&) = delete;
        constexpr RefBase() noexcept = default;
        ~RefBase() = default;

        uint32_t GetStrongRefCount() const noexcept { return shared ? shared->GetStrongRefCount() : 0u; }
        uint32_t GetWeakRefCount() const noexcept { return shared ? shared->GetWeakRefCount() : 0u; }
        T* get() const noexcept { return pointer; }

        void IncrementStrongRef() const noexcept  { if (shared) shared->IncrementStrongRef(); }
        void DecrementStrongRef() noexcept { if (shared) shared->DecrementStrongRef(); }
        void IncrementWeakRef() const noexcept { if (shared) shared->IncrementWeakRef(); }
        void DecrementWeakRef() noexcept { if (shared) shared->DecrementWeakRef(); }

        template <typename TOther>
        void MoveConstruct(RefBase<TOther>&& other) noexcept
        {
            pointer = other.pointer;
            shared = other.shared;
            other.pointer = nullptr;
            other.shared = nullptr;
        }

        template <typename TOther>
        void CopyConstruct(const Ref<TOther>& other) noexcept
        {
            other.IncrementStrongRef();
            pointer = other.pointer;
            shared = other.shared;
        }

        template <typename TOther>
        void AliasConstruct(const Ref<TOther>& other, T* ptr) noexcept
        {
            other.IncrementStrongRef();
            pointer = ptr;
            shared = other.shared;
        }

        template <typename TOther>
        void AliasMoveConstruct(Ref<TOther>&& other, T* ptr) noexcept 
        {
            pointer = ptr;
            shared = other.shared;
            other.pointer = nullptr;
            other.shared = nullptr;
        }

        template <typename TOther>
        bool ConstructFromWeak(const Weak<TOther>& other) noexcept
        {
            if (other.shared && other.shared->IncrementRefNonZero())
            {
                pointer = other.pointer;
                shared = other.shared;
                return true;
            }

            return false;
        }

        template <typename TOther>
        void WeaklyConstruct(const RefBase<TOther>& other) noexcept 
        { 
            if (other.shared)
            {
                pointer = other.pointer;
                shared = other.shared;
                shared->IncrementWeakRef();
            }
        }

        void Swap(RefBase& other) noexcept 
        { 
            PK::Swap(pointer, other.pointer);
            PK::Swap(shared, other.shared);
        }

        T* pointer{ nullptr };
        SharedObjectBase* shared{ nullptr };
    };

    template<typename T>
    struct Ref : public RefBase<T> 
    {
        static_assert(TIsArray<T> == false, "Ref doesn't support array types.");

        using TBase = RefBase<T>;
        using TWeak = Weak<T>;
        using TBase::get;

        constexpr Ref() noexcept = default;
        constexpr Ref(nullptr_t) noexcept {}

        Ref(SharedObjectBase* ref, T* ptr) noexcept 
        { 
            ref->IncrementStrongRef();
            TBase::pointer = ptr;
            TBase::shared = ref;
        }

        Ref(const Ref& other) noexcept { TBase::CopyConstruct(other); }
        Ref(Ref&& other) noexcept { TBase::MoveConstruct(PK::MoveTemp(other)); }

        template <typename TOther> Ref(const Ref<TOther>& other, T* ptr) noexcept { TBase::AliasConstruct(other, ptr); }
        template <typename TOther> Ref(Ref<TOther>&& other, T* ptr) noexcept { TBase::AliasMoveConstruct(PK::MoveTemp(other), ptr); }
        template <typename TOther, TEnableIf_T<TIsConvertible<TOther*, T*>, int> = 0> Ref(const Ref<TOther>& other) noexcept { TBase::CopyConstruct(other); }
        template <typename TOther, TEnableIf_T<TIsConvertible<TOther*, T*>, int> = 0> Ref(Ref<TOther>&& other) noexcept { TBase::MoveConstruct(PK::MoveTemp(other)); }
        template <typename TOther, TEnableIf_T<TIsConvertible<TOther*, T*>, int> = 0> explicit Ref(const Weak<TOther>& other) { Memory::Assert(TBase::ConstructFromWeak(other), "Ref from weak ctor failed!"); }

        ~Ref() noexcept { TBase::DecrementStrongRef(); }

        Ref& operator=(const Ref& other) noexcept { Ref(other).Swap(*this); return *this; }
        Ref& operator=(Ref&& other) noexcept { Ref(PK::MoveTemp(other)).Swap(*this); return *this; }
        template <typename TOther> Ref& operator=(const Ref<TOther>& other) noexcept { Ref(other).Swap(*this); return *this; }
        template <typename TOther> Ref& operator=(Ref<TOther>&& other) noexcept { Ref(PK::MoveTemp(other)).Swap(*this); return *this; }
        T& operator*() const noexcept { return *get(); }
        T* operator->() const noexcept { return get(); }
        explicit operator bool() const noexcept { return get() != nullptr; }

        void Swap(Ref& other) noexcept { TBase::Swap(other); }
        void Reset() noexcept { Ref().Swap(*this); }
    };

    template<typename T>
    struct Weak : public RefBase<T> 
    {
        using TBase = RefBase<T>;

        constexpr Weak() noexcept {}
        
        Weak(const Weak& other) noexcept { TBase::WeaklyConstruct(other);}
        Weak(Weak&& other) noexcept { TBase::MoveConstruct(PK::MoveTemp(other)); }

        template <typename TOther, TEnableIf_T<TIsConvertible<TOther*, T*>, int> = 0>
        Weak(const Ref<TOther>& other) noexcept { TBase::WeaklyConstruct(other); }

        template <typename TOther, TEnableIf_T<TIsConvertible<TOther*, T*>, int> = 0>
        Weak(const Weak<TOther>& other) noexcept 
        {
            if (other.shared)
            {
                TBase::shared = other.shared;
                TBase::shared->IncrementWeakRef();

                if (TBase::shared->IncrementRefNonZero())
                {
                    TBase::pointer = other.pointer;
                    TBase::shared->DecrementWeakRef();
                }
            }
        }

        template <typename TOther, TEnableIf_T<TIsConvertible<TOther*, T*>, int> = 0>
        Weak(Weak<TOther>&& other) noexcept 
        {
            TBase::shared = other.shared;
            other.shared = nullptr;

            if (TBase::shared && TBase::shared->IncrementRefNonZero())
            {
                TBase::pointer = other.pointer;
                TBase::shared->DecrementStrongRef();
            }

            other.pointer = nullptr;
        }

        ~Weak() noexcept { TBase::DecrementWeakRef(); }

        Weak& operator=(const Weak& other) noexcept { Weak(other).Swap(*this); return *this; }
        Weak& operator=(Weak&& other) noexcept { Weak(PK::MoveTemp(other)).Swap(*this); return *this; }
        template <typename TOther> Weak& operator=(const Weak<TOther>& other) noexcept { Weak(other).Swap(*this); return *this; }
        template <typename TOther> Weak& operator=(Weak<TOther>&& other) noexcept { Weak(PK::MoveTemp(other)).Swap(*this); return *this; }
        template <typename TOther> Weak& operator=(const Ref<TOther>& other) noexcept { Weak(other).Swap(*this); return *this; }

        void Reset() noexcept { Weak{}.Swap(*this); }
        void Swap(Weak& other) noexcept { TBase::Swap(other); }
        bool IsAlive() const noexcept { return TBase::GetStrongRefCount() != 0; }

        Ref<T> Lock() const noexcept 
        { 
            Ref<T> ret;
            (void)ret.ConstructFromWeak(*this);
            return ret;
        }
    };

    template <typename T0, typename D0, typename T1, typename D1> bool operator==(const Unique<T0, D0>& a, const Unique<T1, D1>& b) { return a.get() == b.get(); }
    template <typename T0, typename D0, typename T1, typename D1> bool operator!=(const Unique<T0, D0>& a, const Unique<T1, D1>& b) { return a.get() != b.get(); }
    template <typename T0, typename D0, typename T1, typename D1> bool operator<(const Unique<T0, D0>& a, const Unique<T1, D1>& b) { return a.get() < b.get(); }
    template <typename T0, typename D0, typename T1, typename D1> bool operator>=(const Unique<T0, D0>& a, const Unique<T1, D1>& b) { return a.get() >= b.get(); }
    template <typename T0, typename D0, typename T1, typename D1> bool operator>(const Unique<T0, D1>& a, const Unique<T1, D1>& b) { return a.get() > b.get(); }
    template <typename T0, typename D0, typename T1, typename D1> bool operator<=(const Unique<T0, D1>& a, const Unique<T1, D1>& b) { return a.get() <= b.get(); }
    template <typename T, typename D> bool operator==(const Unique<T, D>& a, nullptr_t) noexcept { return a.get() == nullptr; }
    template <typename T, typename D> bool operator!=(const Unique<T, D>& a, nullptr_t) noexcept { return a.get() != nullptr; }
    template <typename T, typename D> bool operator<(const Unique<T, D>& a, nullptr_t) { return a.get() < static_cast<T*>(nullptr); }
    template <typename T, typename D> bool operator>=(const Unique<T, D>& a, nullptr_t) { return a.get() >= static_cast<T*>(nullptr); }
    template <typename T, typename D> bool operator>(const Unique<T, D>& a, nullptr_t) { return a.get() > static_cast<T*>(nullptr); }
    template <typename T, typename D> bool operator<=(const Unique<T, D>& a, nullptr_t) { return a.get() <= static_cast<T*>(nullptr); }
    template <typename T, typename D> bool operator==(nullptr_t, const Unique<T, D>& b) noexcept { return nullptr == b.get(); }
    template <typename T, typename D> bool operator!=(nullptr_t, const Unique<T, D>& b) noexcept { return nullptr != b.get(); }
    template <typename T, typename D> bool operator<(nullptr_t, const Unique<T, D>& b) { return static_cast<T*>(nullptr) < b.get(); }
    template <typename T, typename D> bool operator>=(nullptr_t, const Unique<T, D>& b) { return static_cast<T*>(nullptr) >= b.get(); }
    template <typename T, typename D> bool operator>(nullptr_t, const Unique<T, D>& b) { return static_cast<T*>(nullptr) > b.get(); }
    template <typename T, typename D> bool operator<=(nullptr_t, const Unique<T, D>& b) { return static_cast<T*>(nullptr) <= b.get(); }

    template <typename T0, typename T1> bool operator==(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() == b.get(); }
    template <typename T0, typename T1> bool operator!=(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() != b.get(); }
    template <typename T0, typename T1> bool operator<(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() < b.get(); }
    template <typename T0, typename T1> bool operator>=(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() >= b.get(); }
    template <typename T0, typename T1> bool operator>(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() > b.get();}
    template <typename T0, typename T1> bool operator<=(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() <= b.get(); }
    template <typename T> bool operator==(const Ref<T>& a, nullptr_t) noexcept { return a.get() == nullptr; }
    template <typename T> bool operator!=(const Ref<T>& a, nullptr_t) noexcept { return a.get() != nullptr; }
    template <typename T> bool operator<(const Ref<T>& a, nullptr_t) noexcept { return a.get() < static_cast<T*>(nullptr); }
    template <typename T> bool operator>=(const Ref<T>& a, nullptr_t) noexcept { return a.get() >= static_cast<T*>(nullptr); }
    template <typename T> bool operator>(const Ref<T>& a, nullptr_t) noexcept { return a.get() > static_cast<T*>(nullptr); }
    template <typename T> bool operator<=(const Ref<T>& a, nullptr_t) noexcept { return a.get() <= static_cast<T*>(nullptr); }
    template <typename T> bool operator==(nullptr_t, const Ref<T>& b) noexcept { return nullptr == b.get(); }
    template <typename T> bool operator!=(nullptr_t, const Ref<T>& b) noexcept { return nullptr != b.get(); }
    template <typename T> bool operator<(nullptr_t, const Ref<T>& b) noexcept { return static_cast<T*>(nullptr) < b.get(); }
    template <typename T> bool operator>=(nullptr_t, const Ref<T>& b) noexcept { return static_cast<T*>(nullptr) >= b.get(); }
    template <typename T> bool operator>(nullptr_t, const Ref<T>& b) noexcept { return static_cast<T*>(nullptr) > b.get(); }
    template <typename T> bool operator<=(nullptr_t, const Ref<T>& b) noexcept { return static_cast<T*>(nullptr) <= b.get(); }

    template<typename T, typename ... Args>
    constexpr Unique<T> CreateUnique(Args&& ... args) noexcept
    {
        return Unique<T>(Memory::New<T>(PK::Forward<Args>(args)...));
    }

    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args) noexcept
    {
        const auto shared = Memory::New<SharedObject<T>>(PK::Forward<Args>(args)...);
        Ref<T> ret;
        ret.pointer = &shared->value;
        ret.shared = shared;
        return ret;
    }

    template<typename T, typename TRef>
    static Ref<T> CreateRefAliased(TRef* shared) noexcept
    {
        Ref<T> ret;
        ret.pointer = &shared->value;
        ret.shared = shared;
        return ret;
    }

    template<typename T0, typename T1>
    static Ref<T0> StaticCastRef(const Ref<T1>& other) noexcept
    {
        return Ref<T0>(other, static_cast<T0*>(other.get()));
    }
}
