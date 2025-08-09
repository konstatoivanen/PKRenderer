#pragma once
#include <vector>
#include <typeindex>
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
        ServiceRegister() : m_services(64u, 2u) {}
        ~ServiceRegister() { Clear(); }

        template <typename T>
        T* Get() 
        {
            auto container = m_services.GetValuePtr(std::type_index(typeid(T)))[0].get();
            return &static_cast<ServiceContainer<T>*>(container)->Instance;
        }

        template<typename T, typename ... Args>
        T* Create(Args&& ... args)
        {
            auto typeIndex = std::type_index(typeid(T));
            auto index = 0u;
            AssertTypeExists(m_services.AddKey(typeIndex, &index), typeIndex);
            auto logIndent = PK::LogScopeIndent(2);
            auto service = new ServiceContainer<T>(std::forward<Args>(args)...);
            m_services[index].value = Unique<Service>(service);
            return &service->Instance;
        }

        void Clear()
        {
            for (auto i = (int32_t)m_services.GetCount() - 1; i >= 0; --i)
            {
                m_services[i].value = nullptr;
            }

            m_services.Clear();
        }

    private:
        void AssertTypeExists(bool exists, std::type_index index);

        FastMap<std::type_index, Unique<Service>> m_services;
    };
}