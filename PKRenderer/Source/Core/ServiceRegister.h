#pragma once
#include <vector>
#include <typeindex>
#include <unordered_map>
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FastMap.h"
#include "Core/CLI/LogScopeIndent.h"

namespace PK
{
    class ServiceRegister : public NoCopy
    {
        struct Service : public NoCopy
        {
            virtual ~Service() = 0;
        };

        template<typename T>
        struct ServiceContainer : Service
        {
            T Instance;

            template<typename ... Args>
            ServiceContainer(Args&& ... args) : Instance(std::forward<Args>(args)...) {}
        };
        

    public:
        ServiceRegister() : m_services(64u) {}
        ~ServiceRegister() { Clear(); }

        template <typename T>
        T* Get() 
        {
            auto container = m_services.GetValueRef(std::type_index(typeid(T)))[0].get();
            return &static_cast<ServiceContainer<T>*>(container)->Instance;
        }

        template<typename T, typename ... Args>
        T* Create(Args&& ... args)
        {
            auto typeIndex = std::type_index(typeid(T));
            auto index = 0u;

            if (!m_services.AddKey(typeIndex, &index))
            {
                AssertTypeExists(typeIndex);
            }

            auto logIndent = PK::LogScopeIndent();
            auto service = new ServiceContainer<T>(std::forward<Args>(args)...);
            auto values = m_services.GetValues();
            values[index] = Scope<Service>(service);
            return &service->Instance;
        }

        void Clear()
        {
            auto values = m_services.GetValues();

            for (auto i = (int32_t)m_services.GetCount() - 1; i >= 0; --i)
            {
                values[i] = nullptr;
            }

            m_services.Clear();
        }

    private:
        void AssertTypeExists(std::type_index index);

        FastMap<std::type_index, Scope<Service>> m_services;
    };
}