#pragma once
#include <memory>

#define PK_STACK_ALLOC(Type, count) reinterpret_cast<Type*>(alloca(sizeof(Type) * count))

#define PK_CONTIGUOUS_ALLOC(Type, count) reinterpret_cast<Type*>(calloc(count, sizeof(Type)));

namespace PK
{
    template<typename T>
    using Unique = std::unique_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Unique<T> CreateUnique(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T> struct Ref;
    template <typename T> struct Weak;

    struct SharedObjectBase
    {
        constexpr SharedObjectBase() noexcept = default;
        SharedObjectBase(const SharedObjectBase&) = delete;
        SharedObjectBase& operator=(const SharedObjectBase&) = delete;
        virtual ~SharedObjectBase() noexcept {}
        virtual void Destroy() noexcept = 0;
        virtual void Delete() noexcept = 0;
        long GetStrongRefCount() const noexcept { return static_cast<long>(referenceCount); }
        long GetWeakRefCount() const noexcept { return static_cast<long>(weakCount); }
        void IncrementWeakRef() noexcept { _MT_INCR(weakCount); }
        void IncrementStrongRef() noexcept { _MT_INCR(referenceCount); }
        void DecrementWeakRef() noexcept { if (_MT_DECR(weakCount) == 0) Delete(); }

        void DecrementStrongRef() noexcept 
        { 
            if (_MT_DECR(referenceCount) == 0)
            {
                Destroy();
                DecrementWeakRef();
            }
        }

        bool IncrementRefNonZero() noexcept
        {
            auto& _Volatile_uses = reinterpret_cast<volatile long&>(referenceCount);
            long _Count = __iso_volatile_load32(reinterpret_cast<volatile int*>(&_Volatile_uses));

            while (_Count != 0) 
            {
                const long _Old_value = _INTRIN_RELAXED(_InterlockedCompareExchange)(&_Volatile_uses, _Count + 1, _Count);
                if (_Old_value == _Count) 
                {
                    return true;
                }

                _Count = _Old_value;
            }

            return false;
        }
        
        unsigned long referenceCount = 1u;
        unsigned long weakCount = 1u;
    };

    template <typename T>
    struct SharedObject : public SharedObjectBase
    {
        template <typename ... Args>
        explicit SharedObject(Args&&... args) : SharedObjectBase() { new(&value) T(std::forward<Args>(args)...); }
        virtual ~SharedObject() noexcept override {}
        virtual void Destroy() noexcept override final { value.~T(); }
        virtual void Delete() noexcept override final { delete this; }
        struct U { constexpr U() noexcept {} };
        union { U unionDefault; T value; };
    };

    template <typename T>
    struct RefBase
    {
        RefBase(const RefBase&) = delete;
        RefBase& operator=(const RefBase&) = delete;
        constexpr RefBase() noexcept = default;
        ~RefBase() = default;

        long GetStrongRefCount() const noexcept { return shared ? shared->GetStrongRefCount() : 0; }
        long GetWeakRefCount() const noexcept { return shared ? shared->GetWeakRefCount() : 0; }
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
            std::swap(pointer, other.pointer);
            std::swap(shared, other.shared);
        }

        T* pointer{ nullptr };
        SharedObjectBase* shared{ nullptr };
    };

    template <typename T>
    struct Ref : public RefBase<T> 
    {
        static_assert(std::is_array<T>::value == false, "Ref doesn't support array types.");

        using TBase = RefBase<T>;
        using TWeak = Weak<T>;
        using TBase::get;

        constexpr Ref() noexcept = default;
        constexpr Ref(std::nullptr_t) noexcept {}

        Ref(SharedObjectBase* ref, T* ptr) noexcept 
        { 
            ref->IncrementStrongRef();
            TBase::pointer = ptr;
            TBase::shared = ref;
        }

        template <typename TOther>
        Ref(const Ref<TOther>& other, T* ptr) noexcept { TBase::AliasConstruct(other, ptr); }

        template <typename TOther>
        Ref(Ref<TOther>&& other, T* ptr) noexcept { TBase::AliasMoveConstruct(std::move(other), ptr); }

        Ref(const Ref& other) noexcept { TBase::CopyConstruct(other); }

        template <typename TOther, std::enable_if_t<std::_SP_pointer_compatible<TOther, T>::value, int> = 0>
        Ref(const Ref<TOther>& other) noexcept { TBase::CopyConstruct(other); }

        Ref(Ref&& other) noexcept { TBase::MoveConstruct(std::move(other)); }

        template <typename TOther, std::enable_if_t<std::_SP_pointer_compatible<TOther, T>::value, int> = 0>
        Ref(Ref<TOther>&& other) noexcept { TBase::MoveConstruct(std::move(other)); }

        template <typename TOther, std::enable_if_t<std::_SP_pointer_compatible<TOther, T>::value, int> = 0>
        explicit Ref(const Weak<TOther>& other)
        {
            if (!TBase::ConstructFromWeak(other))
            {
                throw std::exception("Failed to create a Ref from Weak!");
            }
        }

        ~Ref() noexcept { TBase::DecrementStrongRef(); }

        Ref& operator=(const Ref& other) noexcept { Ref(other).Swap(*this); return *this; }

        template <typename TOther>
        Ref& operator=(const Ref<TOther>& other) noexcept { Ref(other).Swap(*this); return *this; }

        Ref& operator=(Ref&& other) noexcept { Ref(std::move(other)).Swap(*this); return *this; }

        template <typename TOther>
        Ref& operator=(Ref<TOther>&& other) noexcept { Ref(std::move(other)).Swap(*this); return *this; }

        void Swap(Ref& other) noexcept { TBase::Swap(other); }
        void Reset() noexcept { Ref().Swap(*this); }

        T& operator*() const noexcept { return *get(); }
        T* operator->() const noexcept { return get(); }
        explicit operator bool() const noexcept { return get() != nullptr; }
    };

    template <typename T>
    struct Weak : public RefBase<T> 
    {
        using TBase = RefBase<T>;

        constexpr Weak() noexcept {}
        
        Weak(const Weak& other) noexcept { TBase::WeaklyConstruct(other);}

        template <typename TOther, std::enable_if_t<std::_SP_pointer_compatible<TOther, T>::value, int> = 0>
        Weak(const Ref<TOther>& other) noexcept { TBase::WeaklyConstruct(other); }

        Weak(Weak&& other) noexcept { TBase::MoveConstruct(std::move(other)); }

        template <typename TOther, std::enable_if_t<std::_SP_pointer_compatible<TOther, T>::value, int> = 0>
        Weak(const Weak<TOther>& other) noexcept 
        {
            if (other.shared)
            {
                shared = other.shared;
                shared->IncrementWeakRef();

                if (shared->IncrementRefNonZero())
                {
                    pointer = other.pointer;
                    shared->DecrementWeakRef();
                }
            }
        }

        template <typename TOther, std::enable_if_t<std::_SP_pointer_compatible<TOther, T>::value, int> = 0>
        Weak(Weak<TOther>&& other) noexcept 
        {
            shared = other.shared;
            other.shared = nullptr;

            if (shared && shared->IncrementRefNonZero())
            {
                pointer = other.pointer;
                shared->DecrementStrongRef();
            }

            other.pointer = nullptr;
        }

        ~Weak() noexcept { TBase::DecrementWeakRef(); }

        Weak& operator=(const Weak& other) noexcept { Weak(other).Swap(*this); return *this; }

        template <typename TOther>
        Weak& operator=(const Weak<TOther>& other) noexcept { Weak(other).Swap(*this); return *this; }

        Weak& operator=(Weak&& other) noexcept { Weak(std::move(other)).Swap(*this); return *this; }

        template <typename TOther>
        Weak& operator=(Weak<TOther>&& other) noexcept { Weak(std::move(other)).Swap(*this); return *this; }

        template <typename TOther>
        Weak& operator=(const Ref<TOther>& other) noexcept { Weak(other).Swap(*this); return *this; }

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

    template <typename T0, typename T1> bool operator==(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() == b.get(); }
    template <typename T0, typename T1> bool operator!=(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() != b.get(); }
    template <typename T0, typename T1> bool operator<(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() < b.get(); }
    template <typename T0, typename T1> bool operator>=(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() >= b.get(); }
    template <typename T0, typename T1> bool operator>(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() > b.get();}
    template <typename T0, typename T1> bool operator<=(const Ref<T0>& a, const Ref<T1>& b) noexcept { return a.get() <= b.get(); }
    template <typename T> bool operator==(const Ref<T>& a, nullptr_t) noexcept { return a.get() == nullptr; }
    template <typename T> bool operator==(nullptr_t, const Ref<T>& b) noexcept { return nullptr == b.get(); }
    template <typename T> bool operator!=(const Ref<T>& a, nullptr_t) noexcept { return a.get() != nullptr; }
    template <typename T> bool operator!=(nullptr_t, const Ref<T>& b) noexcept { return nullptr != b.get(); }
    template <typename T> bool operator<(const Ref<T>& a, nullptr_t) noexcept { return a.get() < static_cast<typename T*>(nullptr); }
    template <typename T> bool operator<(nullptr_t, const Ref<T>& b) noexcept { return static_cast<typename T*>(nullptr) < b.get(); }
    template <typename T> bool operator>=(const Ref<T>& a, nullptr_t) noexcept { return a.get() >= static_cast<typename T*>(nullptr); }
    template <typename T> bool operator>=(nullptr_t, const Ref<T>& b) noexcept { return static_cast<typename T*>(nullptr) >= b.get(); }
    template <typename T> bool operator>(const Ref<T>& a, nullptr_t) noexcept { return a.get() > T*>(nullptr); }
    template <typename T> bool operator>(nullptr_t, const Ref<T>& b) noexcept { return static_cast<typename T*>(nullptr) > b.get(); }
    template <typename T> bool operator<=(const Ref<T>& a, nullptr_t) noexcept { return a.get() <= T*>(nullptr); }
    template <typename T> bool operator<=(nullptr_t, const Ref<T>& b) noexcept { return static_cast<typename T*>(nullptr) <= b.get(); }

    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args)
    {
        const auto shared = new SharedObject<T>(std::forward<Args>(args)...);
        Ref<T> ret;
        ret.pointer = &shared->value;
        ret.shared = shared;
        return ret;
    }

    template<typename T, typename TRef, typename ... Args>
    static Ref<T> CreateRefAliased(const TRef* shared, Args&& ... args)
    {
        Ref<T> ret;
        ret.pointer = &shared->value;
        ret.shared = shared;
        return ret;
    }
}
