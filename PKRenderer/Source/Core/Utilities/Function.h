#pragma once
#include "Memory.h"

namespace PK
{
    template<typename>
    struct Function;

    template<typename TRet, typename... Args>
    struct Function<TRet(Args...)>
    {
        struct Context;
        typedef TRet(*TFunc)(Args...);
        typedef TRet(*TCall)(const Context*, Args...);
        typedef void(*TConstruct)(Context*, const Context*);
        typedef void(*TDestroy)(Context*);
        struct Context { union { void* ctx; TFunc func; alignas(void*) uint8_t data[40u]; }; };

        Function() noexcept : m_function(nullptr), m_construct(nullptr), m_destroy(nullptr), m_context() {}
        Function(const Function& other) noexcept : Function() { Copy(other); }
        Function(Function&& other) noexcept : Function() { Move(PK::MoveTemp(other)); }
        Function(TFunc method) noexcept : Function() { Bind(method); }
        
        template<typename T> 
        Function(T lambda) noexcept : Function() { Bind<T>(PK::MoveTemp(lambda)); }
        
        ~Function() { Unbind(); }

        Function& operator=(const Function& other) noexcept { Copy(other); return *this; }
        Function& operator=(Function&& other) noexcept { Move(PK::MoveTemp(other)); return *this; }
        inline bool operator==(const Function& other) const { return m_function == other.m_function && m_context.ctx == other.m_context.ctx; }
        inline bool operator!=(const Function& other) const { return m_function != other.m_function || m_context.ctx != other.m_context.ctx; }
        inline TRet operator()(Args... args) const { return m_function(&m_context, PK::Forward<Args>(args)...); }
        inline bool IsBound() const { return m_function != nullptr; }

        void Bind(TFunc method) noexcept
        {
            Unbind();
            m_context.func = method;
            m_function = &CallStaticPtr;
        }

        template<TRet(*M)(Args ...)>
        void Bind() noexcept
        {
            Unbind();
            m_function = &CallStatic<M>;
        }

        template<typename T, TRet(T::* M)(Args ...)>
        void Bind(T* context) noexcept
        {
            Unbind();
            m_context.ctx = context;
            m_function = &CallMember<T, M>;
        }

        template<typename T>
        void Bind(T&& lambda) noexcept
        {
            static_assert(sizeof(T) <= sizeof(Context), "Lambda is too big to capture into inline memory!");
            
            Unbind();

            if constexpr (__is_convertible_to(T, TFunc))
            {
                m_context.func = lambda;
                m_function = &CallStaticPtr;
            }
            else
            {
                m_function = reinterpret_cast<TCall>(&CallLambda<T>);
                m_construct = reinterpret_cast<TConstruct>(&ConstructLambda<T>);
                m_destroy = reinterpret_cast<TDestroy>(&DestroyLambda<T>);
                Memory::Construct<T>(m_context.data, PK::Forward<T>(lambda));
            }
        }

        void Unbind()
        {
            if (m_destroy)
            {
                m_destroy(&m_context);
            }

            m_function = nullptr;
            m_destroy = nullptr;
            m_construct = nullptr;
        }

    private:
        inline void Copy(const Function& other)
        {
            if (this != &other)
            {
                Unbind();
                m_function = other.m_function;
                m_construct = other.m_construct;
                m_destroy = other.m_destroy;
                m_context = other.m_context;

                if (m_construct)
                {
                    m_construct(&m_context, &other.m_context);
                }
            }
        }

        inline void Move(Function&& other)
        {
            if (this != &other)
            {
                Copy(other);
                other.Unbind();
            }
        }
        
        static TRet CallStaticPtr(const Context* ctx, Args... args) { return ctx->func(PK::Forward<Args>(args)...); }

        template<TRet(*M)(Args...)> 
        static TRet CallStatic(const Context*, Args... args) { return (M)(PK::Forward<Args>(args)...); }
        
        template<typename T, TRet(T::* M)(Args...)> 
        static TRet CallMember(const Context* ctx, Args ... args) { return (static_cast<T*>(ctx->ctx)->*M)(PK::Forward<Args>(args)...); }
       
        template<typename T, TRet(T::* M)(Args...) const> 
        static TRet CallMember(const Context* ctx, Args ... args) { return (static_cast<T*>(ctx->ctx)->*M)(PK::Forward<Args>(args)...); }
        
        template<typename T> 
        static TRet CallLambda(const T* ctx, Args ... args) { return (*ctx)(PK::Forward<Args>(args)...); }
        
        template<typename T> 
        static void ConstructLambda(T* ctx, const T* lambda) { Memory::Construct<T>(ctx, *lambda); }
        
        template<typename T> 
        static void DestroyLambda(T* ctx) { Memory::Destruct<T>(ctx); }

        TCall m_function;
        TConstruct m_construct;
        TDestroy m_destroy;
        Context m_context;
    };
}
