#pragma once
#include "Memory.h"

namespace PK
{
    struct ILambda
    {
        virtual ILambda* Copy(void* dest) = 0;
        virtual void Destroy() = 0;
    };

    // Abstract lambdas with virtual calls as we dont want to waste 16b of space for extra fn ptrs.
    template<typename T>
    struct Lambda : ILambda
    {
        T lambda;
        Lambda(const T& _lambda) : lambda(_lambda) {}
        ILambda* Copy(void* dest) final { return Memory::Construct<Lambda>(dest, lambda); }
        void Destroy() final { lambda.~T(); }
    };

    template<typename>
    struct Function;

    template<typename TRet, typename... Args>
    struct Function<TRet(Args...)>
    {
        constexpr static const size_t inline_count = 7u;
        constexpr static const size_t back_index = inline_count - 1u;

        union Storage;
        typedef TRet(*TFunc)(Args...);
        typedef TRet(*TCall)(const Storage*, Args...);

        union Storage
        {
            max_align_t align;
            TFunc func;
            void* object;
            char data[sizeof(ILambda*) * inline_count];
            ILambda* ptrs[inline_count];
        };

        Function() noexcept : function(nullptr), storage() {}
        Function(const Function& other) noexcept : Function() { Copy(other); }
        Function(Function&& other) noexcept : Function() { Move(PK::MoveTemp(other)); }
        Function(TFunc method) noexcept : Function() { Bind(method); }
        
        template<typename T> 
        Function(T lambda) noexcept : Function() { Bind<T>(PK::MoveTemp(lambda)); }
        
        ~Function() { Unbind(); }

        Function& operator=(const Function& other) noexcept { Copy(other); return *this; }
        Function& operator=(Function&& other) noexcept { Move(PK::MoveTemp(other)); return *this; }
        inline bool operator==(const Function& other) const { return function == other.function && storage.object == other.storage.object; }
        inline bool operator!=(const Function& other) const { return function != other.function || storage.object != other.storage.object; }
        inline TRet operator()(Args... args) const { return function(&storage, PK::Forward<Args>(args)...); }
        inline bool IsBound() const { return function != nullptr; }

        inline void Copy(const Function& other) noexcept
        {
            if (this != &other)
            {
                Unbind();
                function = other.function;
                storage = other.storage;

                if (other.storage.ptrs[back_index])
                {
                    storage.ptrs[back_index] = other.storage.ptrs[back_index]->Copy(&storage);
                }
            }
        }

        inline void Move(Function&& other) noexcept
        {
            if (this != &other)
            {
                Copy(other);
                other.Unbind();
            }
        }

        template<TRet(*M)(Args ...)>
        void Bind() noexcept
        {
            Unbind();
            function = &CallStatic<M>;
        }

        void Bind(TFunc method) noexcept
        {
            Unbind();
            function = &CallStaticPtr;
            storage.func = method;
        }

        template<typename T, TRet(T::* M)(Args ...)>
        void Bind(T* object) noexcept
        {
            Unbind();
            function = &CallMember<T, M>;
            storage.object = object;
        }

        template<typename T>
        void Bind(T&& lambda) noexcept
        {
            static_assert(sizeof(T) <= sizeof(storage) - sizeof(void*), "Lambda is too big to capture into inline memory!");
            
            Unbind();

            if constexpr (__is_convertible_to(T, TFunc))
            {
                function = &CallStaticPtr;
                storage.func = lambda;
            }
            else
            {
                function = &CallLambda<T>;
                storage.ptrs[back_index] = Memory::Construct<Lambda<T>>(&storage, lambda);
            }
        }

        void Unbind()
        {
            if (storage.ptrs[back_index])
            {
                storage.ptrs[back_index]->Destroy();
                storage.ptrs[back_index] = nullptr;
            }

            function = nullptr;
        }

        static TRet CallStaticPtr(const Storage* s, Args... args) { return s->func(PK::Forward<Args>(args)...); }

        template<TRet(*M)(Args...)>
        static TRet CallStatic(const Storage*, Args... args) { return (M)(PK::Forward<Args>(args)...); }

        template<typename T, TRet(T::* M)(Args...)>
        static TRet CallMember(const Storage* s, Args ... args) { return (static_cast<T*>(s->object)->*M)(PK::Forward<Args>(args)...); }

        template<typename T, TRet(T::* M)(Args...) const>
        static TRet CallMember(const Storage* s, Args ... args) { return (static_cast<T*>(s->object)->*M)(PK::Forward<Args>(args)...); }

        template<typename T>
        static TRet CallLambda(const Storage* s, Args... args) { static_cast<Lambda<T>*>(s->ptrs[back_index])->lambda(PK::Forward<Args>(args)...); }

        TCall function;
        Storage storage;
    };
}
